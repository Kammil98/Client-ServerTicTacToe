#include <stdbool.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <netdb.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <sys/msg.h>
#include <pthread.h>

#define KEY 1234
#define SERVER_PORT 1234
#define MAX_GAMES 100
#define QUEUE_SIZE 5 // this is ignored anyway
#define END_OF_MSG '-'


//structure to keep all ipcs id
struct ipcid{
  int semid;
  int shmid;
  int msgid;
};

//structure to keep date which we want to send to thread
struct thread_data_t{
  int conn_sct_dsc[2];
  struct ipcid ipcid;
};

static struct sembuf buf;

//structure for message
struct msgbuf{
  long mtype;
  int conn_sct_dsc;
};

//structure for semaphore
struct buf_elem {
   long mtype;
   int mvalue;
};


void *StartGame(void *t_data);


/*
semid - id of semaphore table
semnum - number of semaphore in semaphore table
semoper - amount of unlocked space
*/
void mutex_unlock(int semid, int semnum, int semoper){
    buf.sem_num = semnum;
    buf.sem_op = semoper;
    buf.sem_flg = 0;
    if (semop(semid, &buf, 1) == -1){
        perror("Błąd przy próbie podnoszenia semafora");
        exit(1);
    }
}


/*
semid - id of semaphore table
semnum - number of semaphore in semaphore table
semoper - amount of locked space
*/
void mutex_lock(int semid, int semnum, int semoper){
    buf.sem_num = semnum;
    buf.sem_op = -1 * semoper;
    buf.sem_flg = 0;
    if (semop(semid, &buf, 1) == -1){
    perror("Błąd przy próbie opuszczenia semafora");
    printf("%d\n", semid);
        exit(1);
    }
}


/*
semid - int sent to function to receive
id of semaphores
*/
void createIPC(struct ipcid *ipcid){
	//creating IPC objects
  (*ipcid).semid = semget(KEY, MAX_GAMES, IPC_CREAT|0600);
  if((*ipcid).semid == -1){
    perror("Blad przy utworzeniu zbioru semaforow\n");
		exit(1);
	}
  if(semctl( (*ipcid).semid, 0, SETVAL, 0) == -1){
    perror("Nadanie wartosci semaforowi nr 0");
    exit(1);
  }
  for(int i = 1; i < MAX_GAMES; i++){
    if(semctl( (*ipcid).semid, i, SETVAL, 1) == -1){
  		perror("Nadanie wartosci semaforowi nr " + i);
  		exit(1);
  	}
  }
  //shared memory keep conn_sct for all clients, who wait for game
  (*ipcid).shmid = shmget(KEY, (MAX_GAMES * 2 + 1) *sizeof(int), IPC_CREAT|0600);

  if((*ipcid).shmid == -1){
        perror("Blad przy utworzeniu pamięci współdzielonej \nByć może taki klucz juz istnieje dla pamieci wspoldzielonej\n");
  }
  //msg Queue keep conn_sct for all clients, who wait for game
  (*ipcid).msgid = msgget(KEY, IPC_CREAT|0600);
  if((*ipcid).msgid == -1){
    perror("Blad przy utworzeniu kolejki komunikatów\n");
		exit(1);
	}
}


/*
release memory after end of process
*/
void releaseIPC(struct ipcid ipcid){
  if (shmctl (ipcid.semid, IPC_RMID, NULL) < 0)
  {
    perror("Blad przy zwolnieniu zbioru semaforów");
    exit (-1);
  }
  if (shmctl (ipcid.shmid, IPC_RMID, NULL) < 0)
  {
    perror("Blad przy zwolnieniu pamięci współdzielonej");
    exit (-1);
  }
  if (shmctl (ipcid.msgid, IPC_RMID, NULL) < 0)
  {
    perror("Blad przy zwolnieniu kolejki komunikatów");
    exit (-1);
  }
}


/*
Prepare socket for connection with Clients
*/
int prepare_socket(char* argv[]){
  int server_sct_dsc;
  int bind_result;
  int listen_result;
  char reuse_addr_val = 1;
  struct sockaddr_in server_addr;
  //int tab[3];

  //inicjalizacja gniazda serwera
  memset(&server_addr, 0, sizeof(struct sockaddr));
  server_addr.sin_family = AF_INET;
  server_addr.sin_addr.s_addr = htonl(INADDR_ANY); // take from all clients. No matter which addres
  server_addr.sin_port = htons(SERVER_PORT);

  server_sct_dsc = socket(AF_INET, SOCK_STREAM, 0);
  if (server_sct_dsc < 0)
  {
      fprintf(stderr, "%s: Błąd przy próbie utworzenia gniazda\n", argv[0]);
      exit(1);
  }
  setsockopt(server_sct_dsc, SOL_SOCKET, SO_REUSEADDR, (char*)&reuse_addr_val, sizeof(reuse_addr_val));

  bind_result = bind(server_sct_dsc, (struct sockaddr*)&server_addr, sizeof(struct sockaddr));
  if (bind_result < 0)
  {
      fprintf(stderr, "%s: Błąd przy próbie dowiązania adresu IP i numeru portu do gniazda.\n", argv[0]);
      close(server_sct_dsc);
      exit(1);
  }

  listen_result = listen(server_sct_dsc, QUEUE_SIZE);
  if (listen_result < 0) {
      fprintf(stderr, "%s: Błąd przy próbie ustawienia wielkości kolejki.\n", argv[0]);
      close(server_sct_dsc);
      exit(1);
  }
 return server_sct_dsc;
}


/*
search messages from previous call of read which were not handled
return true if msg is full or false if newMsg is not full
*/
bool searchLastMsgs(char *newMsg, char *lastMsg){
  char* eofIndex;
  if(strlen(lastMsg) > 0){//if there is messages from last read
    eofIndex = strchr(lastMsg, '\n');
    if(eofIndex == NULL){// if whole last messages is part of one message
      strcat(newMsg, lastMsg);
      strcpy(lastMsg, "");
      return false;
    }
    else{//found whole message from last read
      *eofIndex = '\0';
      strcat(newMsg, lastMsg);
      strcpy(lastMsg, eofIndex + sizeof(char));
      return true;
    }
  }
  return false;
}


/*
Read messages received from Client
lastMsg - keep last message, wchich wasn't handled
newMsg -- will receive next message
Legend of messages:
r - answer for replay: ry want replay rn end of connection
0-8 - want to make move on this position
e - client closed connection
*/
void readMsg(int conn_sct_dsc, char* newMsg, char* lastMsg, char* buffer){
  int count;
  memset(buffer, '\0', sizeof(char) * 11);
  memset(newMsg, '\0', sizeof(char) * 11);
  if(searchLastMsgs(newMsg, lastMsg)){
    return;
  }
  count = read(conn_sct_dsc, buffer, sizeof(char) * 10);
  if(count < 0){
    fprintf(stderr, "Błąd przy próbie Odczytania danych od Klienta o deskryptorze %d.\n", conn_sct_dsc);
    exit(-1);
  }
  if(count == 0){
    printf("Klient zakończył grę\n");
    fflush(stdout);
    newMsg[0] = 'e';//end
    newMsg[1] = '\n';
    newMsg[2] = '\0';
  }
  else{
    printf("buffer = %s\n", buffer);
    fflush(stdout);
    strcpy(lastMsg, buffer);
    if(searchLastMsgs(newMsg, lastMsg))
      return;
    strcpy(lastMsg, newMsg);
    strcpy(newMsg, "");
  }
}


/*
Write message to Client
*/
void writeMsg(int conn_sct_dsc, char* message, int size){
  message[size] = '\n';
  size ++;
  int count = 0;
  int temp_count = 0;
  while(count != size){
    printf("writeMsg wysyła\n");
    fflush(stdout);
    temp_count = write(conn_sct_dsc, &message[count], size - count * sizeof(char));
    printf("writeMsg wysłał %d znaków\n", temp_count);
    fflush(stdout);
    if(temp_count < 0){
      fprintf(stderr, "Błąd przy próbie wysłania danych do klienta o deskryptorze %d.\n", conn_sct_dsc);
      fflush(stdout);
      exit(-1);
    }
    count += temp_count;
  }
}


/*
Send the same message to both players
*/
void sendToPlayers(int conn_sct_dsc[2], char msg[]){
  int size = sizeof(char) * strlen(msg);
  writeMsg(conn_sct_dsc[0], msg, size);
  writeMsg(conn_sct_dsc[1], msg, size);
}


/*
Check if move is proper
and send notyfication to Clients if it is proper
*/
bool checkMove(int conn_sct_dscs[], char *sign, int field, char tab[]){
char secondSign;
  if(*sign == 'x')
    secondSign = 'o';
  else
    secondSign = 'x';
  char msg[11];
  memset(msg, '\0', sizeof(char) * 11);
  if(tab[field] == '-'){
    tab[field] = *sign;
    //send notyfication about move
    msg[0] = 's';
    msg[1] = *sign;
    msg[2] = (char)field + '0';
    msg[3] = '\0';
    printf("Wysyłam %s\n",  msg);
    fflush(stdout);
    sendToPlayers(conn_sct_dscs, msg);

    //send notyfication about change turn
    *sign = secondSign;
    msg[0] = 't';
    msg[1] = *sign;
    msg[2] = '\0';
    printf("Wysyłam %s\n",  msg);
    fflush(stdout);
    sendToPlayers(conn_sct_dscs, msg);
    return true;
  }
  else{
    printf("field = %d\ntab[field] = %c\n", field, tab[field]);
    fflush(stdout);
  }
  return false;
}


/*
return character of winner if someone won
or 'n' if none won
or 'd' if there is a draw
*/
char checkWinCondition(char tab[]){
  for(int i = 0; i <3; i++){
    if(tab[i] != '-' && tab[i] == tab[i + 3] && tab[i + 3] == tab[i + 6])//vertical line
      return tab[i];
    if(tab[i * 3] != '-' && tab[i * 3] == tab[i * 3 + 1] && tab[i * 3 + 1] == tab[i * 3 + 2])//horizontal line
      return tab[i * 3];
  }
  if(tab[0] != '-' && tab[0] == tab[4] && tab[4] == tab[8])
    return tab[0];
  if(tab[2] != '-' && tab[2] == tab[4] && tab[4] == tab[6])
    return tab[2];
  for(int i = 0; i <9; i++){
    if(tab[i] == '-'){
      return 'n';
    }
  }
  return 'd';
}


/*
Create new pthread for Game
*/
void createGameThread(struct thread_data_t t_data){
  struct ipcid ipcid;
  ipcid = t_data.ipcid;
  pthread_t thread;
  struct thread_data_t *gameData;
  int create_result;
  gameData = (struct thread_data_t *)malloc(sizeof(struct thread_data_t));
  (*gameData).conn_sct_dsc[0] = t_data.conn_sct_dsc[0];
  (*gameData).conn_sct_dsc[1] = t_data.conn_sct_dsc[1];
  (*gameData).ipcid = ipcid;
  create_result = pthread_create(&thread, NULL, StartGame, (void *)gameData);
  if (create_result){
     printf("Błąd przy próbie utworzenia wątku dobierajacego w pary klientów, kod błędu: %d\n", create_result);
     exit(-1);
  }
}


/*
Send clients message who is the winner
Ask them for replay and create next game
if both want replay.
If only one want reply, then send him to Queue
of waiting players
*/
void endGame(struct thread_data_t th_data, char won, char lastMsg[2][11]){
  char msg[2][11];
  char buffer[11];
  memset(buffer, '\0', sizeof(char) * 11);
  //send winner and set, that it is noone turn
  strcpy(msg[0], "tn");
  printf("Wysyłam %s\n",  msg[0]);
  sendToPlayers(th_data.conn_sct_dsc, msg[0]);
  msg[0][0] = 'w';
  msg[0][1] = won;
  msg[0][2] = '\0';
  printf("Wysyłam %s\n",  msg[0]);
  if(won != 'O')//if client 0 do not loose by connection lost
    writeMsg(th_data.conn_sct_dsc[0], msg[0], sizeof(char) * strlen(msg[0]));
  if(won != 'X')//if client 1 do not loose by connection lost
    writeMsg(th_data.conn_sct_dsc[1], msg[0], sizeof(char) * strlen(msg[0]));
  //sendToPlayers(th_data.conn_sct_dsc, msg[0]);

  //Read answer for question does they want a replay
  readMsg(th_data.conn_sct_dsc[0], msg[0], lastMsg[0], buffer);
  if(msg[0][0] != 'r' && msg[0][0] != 'e'){//if not replay answer and not end connection
    printf("Błąd przy próbie odczytania wiadomości o ponownej rozgrywce od klienta o deskryptorze %d\n", th_data.conn_sct_dsc[0]);
    printf("msg=%s\n", msg[0]);
    exit(-1);
  }
  readMsg(th_data.conn_sct_dsc[1], msg[1], lastMsg[1], buffer);
  printf("0=%s\n1=%s\n", msg[0], msg[1]);
  if(msg[1][0] != 'r' && msg[1][0] != 'e'){//if not replay answer and not end connection
    printf("Błąd przy próbie odczytania wiadomości o ponownej rozgrywce od klienta o deskryptorze %d\n", th_data.conn_sct_dsc[0]);
    printf("msg=%s\n", msg[1]);
    exit(-1);
  }
  else{
    if(strcmp(msg[0], "ry") == 0 && strcmp(msg[1], "ry") == 0){
      createGameThread(th_data);
    }
    else{
      struct msgbuf sender;
      sender.mtype = 1;
      int msgSucces;
      for(int i = 0; i < 2; i++){
        if(strcmp(msg[i], "ry") == 0){
          sender.conn_sct_dsc = th_data.conn_sct_dsc[i];
          mutex_lock(th_data.ipcid.semid, 1, 1);//block opssibility of sending messages
          printf("Send nr 0 to Queue\n");
          fflush(stdout);
          msgSucces = msgsnd(th_data.ipcid.msgid, &sender, sizeof(sender.conn_sct_dsc), 0);
          if ( msgSucces == -1){
            printf("Błąd przy próbie wysłania komunikatu\n");
            fflush(stdout);
            exit(1);
          }
          else{
            mutex_unlock(th_data.ipcid.semid, 0, 1);//allow matchClients function to take message
          }
          mutex_unlock(th_data.ipcid.semid, 1, 1);
        }
      }
    }
  }
}


/*
Check moves of clients and send them information,
do they can make such a move and do someone won
At the end of the game send conn_sct_dsc to Queue
if client want to play again
*/
void *StartGame(void *t_data){
  //initializing block
  printf("Tworze nowa gre.\n");
  fflush(stdout);
  if(pthread_detach(pthread_self()) != 0){
    perror("Błąd przy próbie odłączenia wątku StartGame od wątku macierzystego.\n");
    exit(-1);
  }
  struct thread_data_t th_data = *((struct thread_data_t*)t_data);
  free(t_data);
  char turn = 'x', msgToClient[2][11], tab[9];
  char buffer[11], newMsg[11], lastMsg[2][11];
  int field;
  char won = 'n';
  memset(tab, '-', sizeof(char) * 9);
  memset(lastMsg[0], '\0', sizeof(char) * 11);
  memset(lastMsg[1], '\0', sizeof(char) * 11);
  memset(buffer, '\0', sizeof(char) * 11);
  memset(newMsg, '\0', sizeof(char) * 11);
  memset(msgToClient[0], '\0', sizeof(char) * 11);
  memset(msgToClient[1], '\0', sizeof(char) * 11);

  //set who's turn is now
  strcpy(msgToClient[0], "nxx");
  strcpy(msgToClient[1], "nxo");
  writeMsg(th_data.conn_sct_dsc[0], msgToClient[0], sizeof(char) * strlen(msgToClient[0]));
  writeMsg(th_data.conn_sct_dsc[1], msgToClient[1], sizeof(char) * strlen(msgToClient[1]));

  //Game loop
  while(won == 'n'){
    for(int i =0; i <2; i++){
      if(won == 'n'){
        do{
          readMsg(th_data.conn_sct_dsc[i], newMsg, lastMsg[i], buffer);
          if(newMsg[0] != 'e')
            field = (int)newMsg[0] - (int)'0';
          else{
            if(i == 0)
              won = 'O';
            else
              won = 'X';
          }
        }while(!checkMove(th_data.conn_sct_dsc, &turn, field, tab) && newMsg[0] != 'e');
        if(won == 'n')
          won = checkWinCondition(tab);
      }
    }
  }
  endGame(th_data, won, lastMsg);
  pthread_exit(NULL);
}


/*
Match Clients in pairs to play game
take conn_sct_dsc when any arrive in Queue
*/
void *matchClients(void *t_data) {
  if(pthread_detach(pthread_self()) != 0){
    perror("Błąd przy próbie odłączenia wątku matchClients od wątku macierzystego.\n");
    exit(-1);
  }
  struct thread_data_t *gameData;
  struct thread_data_t th_data = *((struct thread_data_t*)t_data);
  free(t_data);
  int clients_dsc[2], i = 0;
  int msgSucces, create_result;
  pthread_t thread;
  struct msgbuf receiver;

  while(1)
  {
    mutex_lock(th_data.ipcid.semid, 0, 1);
    //receive msg of any type( 4 argument == 0 mean any type of msg)
    msgSucces = msgrcv(th_data.ipcid.msgid, &receiver, sizeof(receiver.conn_sct_dsc), 0, 0);
    if(msgSucces < 0){
      perror("Błąd przy próbie odebrania deskryptora połączenia z kolejki w wątku matchClients.\n");
      printf("Argumenty msgid=%d\nsender.conn_sct_dsc=%d\n", th_data.ipcid.msgid, receiver.conn_sct_dsc);
    }
    else{
      clients_dsc[i] = receiver.conn_sct_dsc;
      i++;
      printf("Dodaje klienta do gry.\n");
      fflush(stdout);
    }
    if(i == 2){
      i = 0;
      gameData = (struct thread_data_t *)malloc(sizeof(struct thread_data_t));
      (*gameData).conn_sct_dsc[0] = clients_dsc[0];
      (*gameData).conn_sct_dsc[1] = clients_dsc[1];
      (*gameData).ipcid = th_data.ipcid;
      create_result = pthread_create(&thread, NULL, StartGame, (void *)gameData);
      if (create_result){
         printf("Błąd przy próbie utworzenia wątku dobierajacego w pary klientów, kod błędu: %d\n", create_result);
         exit(-1);
      }
    }
  }
  pthread_exit(NULL);
}


/*
Create thread with matchClients
function
*/
void wakeUpMatchClients(struct ipcid ipcid){
  //uchwyt na wątek
  pthread_t thread1;
  struct thread_data_t *t_data = (struct thread_data_t *)malloc(sizeof(struct thread_data_t));
  (*t_data).ipcid = ipcid;
  int create_result = pthread_create(&thread1, NULL, matchClients, (void *)t_data);
  if (create_result){
     printf("Błąd przy próbie utworzenia wątku dobierajacego w pary klientów, kod błędu: %d\n", create_result);
     releaseIPC(ipcid);
     exit(1);
  }
}


/*
accpt Client when someone want to connect
and send his conn_sct_dsc to Queue
*/
void acceptClients(int server_sct_dsc, struct ipcid ipcid){
  struct msgbuf sender;
  sender.mtype = 1;
  int msgSucces;
  int conn_sct_dsc = -1;

  conn_sct_dsc = accept(server_sct_dsc, NULL, NULL);
  if (conn_sct_dsc < 0)
  {
     fprintf(stderr, "Błąd przy próbie utworzenia gniazda dla połączenia.\n");
     close(server_sct_dsc);
     releaseIPC(ipcid);
     exit(1);
  }
  sender.conn_sct_dsc = conn_sct_dsc;
  mutex_lock(ipcid.semid, 1, 1);//block opssibility of sending messages
  msgSucces = msgsnd(ipcid.msgid, &sender, sizeof(sender.conn_sct_dsc), 0);
  mutex_unlock(ipcid.semid, 1, 1);//block opssibility of sending messages
  if ( msgSucces == -1){
    printf("Argumenty msgid=%d\nsender.conn_sct_dsc=%d\n", ipcid.msgid, sender.conn_sct_dsc);
    perror("Błąd przy próbie wysłania komunikatu");
    close(server_sct_dsc);
    releaseIPC(ipcid);
    exit(1);
  }
  else{
    mutex_unlock(ipcid.semid, 0, 1);//allow matchClients function to take message
  }
}


int main(int argc, char* argv[])
{
  struct ipcid ipcid;
  int server_sct_dsc;
  server_sct_dsc = prepare_socket(argv);
  createIPC(&ipcid);
  wakeUpMatchClients(ipcid);
  while(1)
  {
    acceptClients(server_sct_dsc, ipcid);
  }
  releaseIPC(ipcid);
  close(server_sct_dsc);
  return(0);
}
