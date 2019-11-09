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
void endGame(struct thread_data_t th_data, char won, char lastMsg[2][11]);
void endGameForClient(struct thread_data_t th_data, char won, char lastMsg[11]);


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
      fprintf(stderr, "Błąd przy próbie podniesienia semafora o id: %d\n", semid);
      exit(-1);
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
    fprintf(stderr, "Błąd przy próbie opuszczenia semafora o id: %d\n", semid);
      exit(-1);
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
		exit(-1);
	}
  if(semctl( (*ipcid).semid, 0, SETVAL, 0) == -1){
    perror("Nadanie wartosci semaforowi nr 0");
    exit(-1);
  }
  if(semctl( (*ipcid).semid, 1, SETVAL, 1) == -1){
		perror("Nadanie wartosci semaforowi nr 1");
		exit(-1);
	}
  //msg Queue keep conn_sct for all clients, who wait for game
  (*ipcid).msgid = msgget(KEY, IPC_CREAT|0600);
  if((*ipcid).msgid == -1){
    perror("Blad przy utworzeniu kolejki komunikatów\n");
		exit(-1);
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

  //initialization of server socket
  memset(&server_addr, 0, sizeof(struct sockaddr));
  server_addr.sin_family = AF_INET;
  server_addr.sin_addr.s_addr = htonl(INADDR_ANY); // take from all clients. No matter which addres
  server_addr.sin_port = htons(SERVER_PORT);

  server_sct_dsc = socket(AF_INET, SOCK_STREAM, 0);
  if (server_sct_dsc < 0){
      fprintf(stderr, "Błąd przy próbie utworzenia gniazda: %s\n", strerror(server_sct_dsc));
      exit(-1);
  }
  setsockopt(server_sct_dsc, SOL_SOCKET, SO_REUSEADDR, (char*)&reuse_addr_val, sizeof(reuse_addr_val));

  bind_result = bind(server_sct_dsc, (struct sockaddr*)&server_addr, sizeof(struct sockaddr));
  if (bind_result < 0){
      fprintf(stderr, ": Błąd przy próbie dowiązania adresu IP i numeru portu do gniazda: %s\n", strerror(bind_result));
      close(server_sct_dsc);
      exit(-1);
  }

  listen_result = listen(server_sct_dsc, QUEUE_SIZE);
  if (listen_result < 0) {
      fprintf(stderr, "Błąd przy próbie ustawienia wielkości kolejki: %s\n", strerror(listen_result));
      close(server_sct_dsc);
      exit(-1);
  }
 return server_sct_dsc;
}


/*
send given Client
to Queue of clients
who wait for game
*/
void sentToQueue(struct ipcid ipcid, int conn_sct_dsc){
  struct msgbuf sender;
  sender.mtype = 1;
  int msgSucces;
  sender.conn_sct_dsc = conn_sct_dsc;
  mutex_lock(ipcid.semid, 1, 1);//block possibility of sending messages
  msgSucces = msgsnd(ipcid.msgid, &sender, sizeof(sender.conn_sct_dsc), 0);
  mutex_unlock(ipcid.semid, 1, 1);//unlock opssibility of sending messages
  if ( msgSucces == -1){
    printf("Argumenty msgid=%d\nsender.conn_sct_dsc=%d\n", ipcid.msgid, sender.conn_sct_dsc);
    perror("Błąd przy próbie wysłania komunikatu");
    releaseIPC(ipcid);
    exit(-1);
  }
  else{
    mutex_unlock(ipcid.semid, 0, 1);//allow matchClients function to take message
  }
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
void readMsg(int conn_sct_dsc, char* newMsg, char* lastMsg){
  int count;
  memset(newMsg, '\0', sizeof(char) * 11);
  if(searchLastMsgs(newMsg, lastMsg)){
    return;
  }
  memset(lastMsg, '\0', sizeof(char) * 11);
  count = read(conn_sct_dsc, lastMsg, sizeof(char) * 10);
  if(count < 0){
    fprintf(stderr, "Błąd przy próbie Odczytania danych od Klienta o deskryptorze %d.\n", conn_sct_dsc);
    exit(-1);
  }
  if(count == 0){
    fprintf(stderr, "Klient o deskryptorze %d przerwał grę.\n", conn_sct_dsc);
    newMsg[0] = 'e';//end
    newMsg[1] = '\n';
    newMsg[2] = '\0';
    return;
  }
  else{
    if(searchLastMsgs(newMsg, lastMsg))
      return;
    //if finally did not found full messages
    //then sent all to lastMsg
    strcpy(lastMsg, newMsg);
    strcpy(newMsg, "");
  }
}


/*
Write message to Client
*/
bool writeMsg(int conn_sct_dsc, char* message, int size){
  char readyMsg[11];
  strcpy(readyMsg, message);
  readyMsg[size] = '\n';
  readyMsg[size + 1] = '\0';
  size += sizeof(char);
  int count = 0;
  int error = 0;
  socklen_t len = sizeof (error);
  int temp_count = 0, retval;
  while(count != size){
    //not whole message was send and there is no error with connection
    retval = getsockopt(conn_sct_dsc, SOL_SOCKET, SO_ERROR, &error, &len);
    if(retval < 0){
      fprintf(stderr, "Błąd przy próbie sprawdzenia połączenia z klientem o deksryptorze %d: %s\n", conn_sct_dsc, strerror(retval));
      return false;
    }
    if(error != 0) {
      fprintf(stderr, "Połączenie z klientem o deksryptorze %d zostało zerwane: %s\n", conn_sct_dsc, strerror(error));
      fflush(stderr);
      return false;
    }
    temp_count = write(conn_sct_dsc, &readyMsg[count], size - count * sizeof(char));
    if(temp_count < 0){
      fprintf(stderr, "Błąd przy próbie wysłania danych do klienta o deskryptorze %d.\n", conn_sct_dsc);
      fflush(stderr);
      return false;
    }
    count += temp_count;
  }
  return true;
}


/*
Send the same message to both players
*/
void sendToPlayers(struct thread_data_t th_data, char msg[], char lastUnreadMsg[2][11]){
  int size = sizeof(char) * strlen(msg);
  if(!writeMsg(th_data.conn_sct_dsc[0], msg, size))//if client is disconnected
    endGameForClient(th_data, 'O', lastUnreadMsg[1]);
  if(!writeMsg(th_data.conn_sct_dsc[1], msg, size))
    endGameForClient(th_data, 'X', lastUnreadMsg[0]);
}


/*
Check if move is proper
and send notyfication to Clients if it is proper
*/
bool checkMove(struct thread_data_t th_data, char *sign, int field, char tab[], char lastMsg[2][11]){
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
    sendToPlayers(th_data, msg, lastMsg);

    //send notyfication about change turn
    *sign = secondSign;
    msg[0] = 't';
    msg[1] = *sign;
    msg[2] = '\0';
    sendToPlayers(th_data, msg, lastMsg);
    return true;
  }
  strcpy(msg, "f");
  if(*sign == 'x')
    writeMsg(th_data.conn_sct_dsc[0], msg, sizeof(char));
  else
    writeMsg(th_data.conn_sct_dsc[1], msg, sizeof(char));
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
     printf("Błąd przy próbie utworzenia wątku dobierajacego w pary klientów: %s\n", strerror(create_result));
     exit(-1);
  }
}


/*
Send client message who is the winner
Ask him for replay and
If he want reply, then send him to Queue
of waiting players
*/
void endGameForClient(struct thread_data_t th_data, char won, char lastMsg[11]){
  char msg[11];
  int who_connected;

  //send winner and set, that it is noone turn
  if(won == 'O')
    who_connected = th_data.conn_sct_dsc[1];
  else
    who_connected = th_data.conn_sct_dsc[0];
  strcpy(msg, "tn");
  if(!writeMsg(who_connected, msg, sizeof(char) * strlen(msg)))//If this client also is disconnected
    pthread_exit(NULL);
  msg[0] = 'w';
  msg[1] = won;
  msg[2] = '\0';
  if(!writeMsg(who_connected, msg, sizeof(char) * strlen(msg)))
    pthread_exit(NULL);

  //Ask for join to queue of clients waiting for game
  readMsg(who_connected, msg, lastMsg);
  if(msg[0] != 'r' && msg[0] != 'e'){//if not replay answer and not end connection
    printf("Błąd przy próbie odczytania wiadomości o ponownej rozgrywce od klienta o deskryptorze %d\n", th_data.conn_sct_dsc[0]);
    printf("Odczytano msg=%s\n", msg);
    exit(-1);
  }
  if(strcmp(msg, "ry") == 0){
    strcpy(msg, "Sn");
    if(!writeMsg(who_connected, msg, sizeof(char) * strlen(msg)))
      pthread_exit(NULL);
    sentToQueue(th_data.ipcid, who_connected);
  }
  pthread_exit(NULL);
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
  bool wantReplay[2];
  memset(wantReplay, false, sizeof(bool) * 2);
  //send winner and set, that it is noone turn
  strcpy(msg[0], "tn");
  sendToPlayers(th_data, msg[0], lastMsg);
  msg[0][0] = 'w';
  msg[0][1] = won;
  msg[0][2] = '\0';
  sendToPlayers(th_data, msg[0], lastMsg);

  //Read answer for question does they want a replay
  for(int i = 0; i < 2; i++){
    readMsg(th_data.conn_sct_dsc[i], msg[i], lastMsg[i]);
    if(msg[i][0] != 'r' && msg[i][0] != 'e'){//if not replay answer and not end connection
      fprintf(stderr, "Błąd przy próbie odczytania wiadomości o ponownej rozgrywce od klienta o deskryptorze %d\nOdebrano msg=%s\n", th_data.conn_sct_dsc[i], msg[i]);
      exit(-1);
    }
    if(strcmp(msg[i], "ry") == 0){
      wantReplay[i] = true;
      strcpy(msg[0], "Sn");
      if(!writeMsg(th_data.conn_sct_dsc[i], msg[0], sizeof(char) * strlen(msg[0])))
        pthread_exit(NULL);
    }
  }
  if(wantReplay[0] == wantReplay[1] && wantReplay[0] == true)
    createGameThread(th_data);
  else{
    for(int i = 0; i < 2; i++){
      if(wantReplay[i])
        sentToQueue(th_data.ipcid, th_data.conn_sct_dsc[i]);
    }
  }
  pthread_exit(NULL);
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
  char newMsg[11], lastMsg[2][11];
  int field;
  char won = 'n';
  memset(tab, '-', sizeof(char) * 9);
  memset(lastMsg[0], '\0', sizeof(char) * 11);
  memset(lastMsg[1], '\0', sizeof(char) * 11);
  memset(newMsg, '\0', sizeof(char) * 11);
  memset(msgToClient[0], '\0', sizeof(char) * 11);
  memset(msgToClient[1], '\0', sizeof(char) * 11);

  //set who's turn is now
  strcpy(msgToClient[0], "nxx");
  strcpy(msgToClient[1], "nxo");
  if(!writeMsg(th_data.conn_sct_dsc[0], msgToClient[0], sizeof(char) * strlen(msgToClient[0]) ))
    endGameForClient(th_data, 'O', lastMsg[1]);
  if(!writeMsg(th_data.conn_sct_dsc[1], msgToClient[1], sizeof(char) * strlen(msgToClient[1]) ))
    endGameForClient(th_data, 'X', lastMsg[0]);
  //Game loop
  while(won == 'n'){
    for(int i = 0; i < 2; i++){
      if(won == 'n'){
        do{
          readMsg(th_data.conn_sct_dsc[i], newMsg, lastMsg[i]);
          if(newMsg[0] != 'e')
            field = (int)newMsg[0] - (int)'0';
          else{
            if(i == 0)
              endGameForClient(th_data, 'O', lastMsg[1]);
            else
              endGameForClient(th_data, 'X', lastMsg[0]);
          }
        }while(!checkMove(th_data, &turn, field, tab, lastMsg) && newMsg[0] != 'e');
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
  struct thread_data_t th_data = *((struct thread_data_t*)t_data);
  free(t_data);
  int msgSucces, i = 0;
  struct msgbuf receiver;

  while(1){
    mutex_lock(th_data.ipcid.semid, 0, 1);
    //receive msg of any type( 4 argument == 0 mean any type of msg)
    mutex_lock(th_data.ipcid.semid, 1, 1);
    msgSucces = msgrcv(th_data.ipcid.msgid, &receiver, sizeof(receiver.conn_sct_dsc), 0, 0);
    mutex_unlock(th_data.ipcid.semid, 1, 1);
    if(msgSucces < 0){
      perror("Błąd przy próbie odebrania deskryptora połączenia z kolejki w wątku matchClients.\n");
      printf("Argumenty msgid=%d\nsender.conn_sct_dsc=%d\n", th_data.ipcid.msgid, receiver.conn_sct_dsc);
      exit(-1);
    }
    else
      th_data.conn_sct_dsc[i++] = receiver.conn_sct_dsc;
    if(i == 2){
      i = 0;
      createGameThread(th_data);
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
     fprintf(stderr, "Błąd przy próbie utworzenia wątku dobierajacego w pary klientów: %s\n", strerror(create_result));
     releaseIPC(ipcid);
     exit(1);
  }
}


/*
accpt Client when someone want to connect
and send his conn_sct_dsc to Queue
*/
void acceptClients(int server_sct_dsc, struct ipcid ipcid){
  int conn_sct_dsc = -1;

  conn_sct_dsc = accept(server_sct_dsc, NULL, NULL);
  if (conn_sct_dsc < 0)
  {
     fprintf(stderr, "Błąd przy próbie utworzenia gniazda dla połączenia.\n");
     close(server_sct_dsc);
     releaseIPC(ipcid);
     exit(-1);
  }
  sentToQueue(ipcid, conn_sct_dsc);
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
