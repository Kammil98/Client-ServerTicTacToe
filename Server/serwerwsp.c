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
#define SERVER_PORT 1235
#define MAX_GAMES 100
#define QUEUE_SIZE 5 // this is ignored anyway
#define END_OF_MSG '-'


//struktura zawierająca dane, które zostaną przekazane do wątku
struct thread_data_t
{
  int conn_sct_dsc;
  int tab[3];
  int semid;
  int shmid;
  int msgid;
};

//structure for message
struct msgbuf
{
  long mtype;
  int conn_sct_dsc;
};

static struct sembuf buf2;

//structure for semaphore
struct buf_elem {
   long mtype;
   int mvalue;
};


/*
semid - id of semaphore table
semnum - number of semaphore in semaphore table
semoper - amount of unlocked space
*/
void mutex_unlock(int semid, int semnum, int semoper){
    buf2.sem_num = semnum;
    buf2.sem_op = semoper;
    buf2.sem_flg = 0;
    if (semop(semid, &buf2, 1) == -1){
        perror("Podnoszenie semafora");
        exit(1);
    }
}


/*
semid - id of semaphore table
semnum - number of semaphore in semaphore table
semoper - amount of locked space
*/
void mutex_lock(int semid, int semnum, int semoper){
    buf2.sem_num = semnum;
    buf2.sem_op = -1 * semoper;
    buf2.sem_flg = 0;
    if (semop(semid, &buf2, 1) == -1){
    perror("Opuszczenie semafora");
    printf("%d\n", semid);
        exit(1);
    }
}


/*
semid - int sent to function to receive
id of semaphores
*/
void createIPC(int *semid, int *shmid, int *msgid){
	//creating IPC objects
  *semid = semget(KEY, MAX_GAMES, IPC_CREAT|0600);
  if(*semid == -1){
    perror("Blad przy utworzeniu zbioru semaforow\n");
		exit(1);
	}
  if(semctl( *semid, 0, SETVAL, 0) == -1){
    perror("Nadanie wartosci semaforowi nr 0");
    exit(1);
  }
  for(int i = 1; i < MAX_GAMES; i++){
    if(semctl( *semid, i, SETVAL, 1) == -1){
  		perror("Nadanie wartosci semaforowi nr " + i);
  		exit(1);
  	}
  }
  //shared memory keep conn_sct for all clients, who wait for game
  *shmid = shmget(KEY, (MAX_GAMES * 2 + 1) *sizeof(int), IPC_CREAT|0600);

  if(*shmid == -1){
        perror("Blad przy utworzeniu pamięci współdzielonej \nByć może taki klucz juz istnieje dla pamieci wspoldzielonej\n");
  }
  //msg Queue keep conn_sct for all clients, who wait for game
  *msgid = msgget(KEY, IPC_CREAT|0600);
  if(*msgid == -1){
    perror("Blad przy utworzeniu kolejki komunikatów\n");
		exit(1);
	}
}


/*
release memory after end of process
*/
void releaseIPC(int semid, int shmid, int msgid){
  if (shmctl (semid, IPC_RMID, NULL) < 0)
  {
    perror("Blad przy zwolnieniu zbioru semaforów");
    exit (-1);
  }
  if (shmctl (shmid, IPC_RMID, NULL) < 0)
  {
    perror("Blad przy zwolnieniu pamięci współdzielonej");
    exit (-1);
  }
  if (shmctl (msgid, IPC_RMID, NULL) < 0)
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


//funkcja opisującą zachowanie wątku - musi przyjmować argument typu (void *) i zwracać (void *)
void *ThreadBehavior(void *t_data){
  if(pthread_detach(pthread_self()) != 0){
    perror("Błąd przy próbie odłączenia wątku ThreadBehavior od wątku macierzystego.\n");
    exit(-1);
  }
  //struct thread_data_t *th_data = (struct thread_data_t*)t_data;
  //dostęp do pól struktury: (*th_data).pole
  //TODO (przy zadaniu 1) klawiatura -> wysyłanie albo odbieranie -> wyświetlanie
  //int clients_dsc[2];
  //int i = 0;
  //char buf[100];
  while(1)
  {
  }
  pthread_exit(NULL);
}


//funkcja obsługująca połączenie z nowym klientem
void handleConnection(int conn_sct_dsc) {
  //wynik funkcji tworzącej wątek
  int create_result = 0;
  //char buf[100];

  //uchwyt na wątek
  pthread_t thread1;

  struct thread_data_t t_data;
  t_data.conn_sct_dsc = conn_sct_dsc;
  //for(int i=0;i<3;i++)
  //  t_data.tab[i]=tab[i];
  //dane, które zostaną przekazane do wątku
  //TODO dynamiczne utworzenie instancji struktury thread_data_t o nazwie t_data (+ w odpowiednim miejscu zwolnienie pamięci)

  //TODO wypełnienie pól struktury

  create_result = pthread_create(&thread1, NULL, ThreadBehavior, (void *)&t_data);
  if (create_result){
     printf("Błąd przy próbie utworzenia wątku, kod błędu: %d\n", create_result);
     exit(-1);
  }
/*  while(1)
  {
    for(int i=0;i<3;i++){
      for(int i=0;i<100;i++)
      	buf[i]=' ';

    read(tab[i],buf,sizeof(buf));
    for(int j=0;j<3;j++){
      if(i!=j)
        write(t_data.tab[j],buf,sizeof(buf));}
      printf("%p", &buf);
      fflush(stdout);
    }
  }*/

  //TODO (przy zadaniu 1) odbieranie -> wyświetlanie albo klawiatura -> wysyłanie
}


/*
Match Clients in pairs
to play game
*/
void *matchClients(void *t_data) {
  if(pthread_detach(pthread_self()) != 0){
    perror("Błąd przy próbie odłączenia wątku matchClients od wątku macierzystego.\n");
    exit(-1);
  }
  struct thread_data_t th_data = *((struct thread_data_t*)t_data);
  int clients_dsc[2], i = 0;
  int msgSucces;
  struct msgbuf receiver;

  while(1)
  {
    mutex_lock(th_data.semid, 0, 1);
    //receive msg of any type( 4 argument == 0 mean any type of msg)
    msgSucces = msgrcv(th_data.msgid, &receiver, sizeof(receiver.conn_sct_dsc), 0, 0);
    if(msgSucces < 0){
      perror("Błąd przy próbie odebrania deskryptora połączenia z kolejki w wątku matchClients.\n");
      printf("Argumenty msgid=%d\nsender.conn_sct_dsc=%d\n", th_data.msgid, receiver.conn_sct_dsc);
    }
    else{
      clients_dsc[i] = receiver.conn_sct_dsc;
      i++;
      printf("Dodaje klienta do gry.\n");
      fflush(stdout);
    }
    if(i == 2){
      i = 0;
      printf("Tworze nowa gre.\n");
      fflush(stdout);
      //startGame(clients_dsc);
    }
  }
  pthread_exit(NULL);
}


/*
Create thread with matchClients
function
*/
void wakeUpMatchClients(int semid, int msgid){
  printf("%d\n", semid);
  //uchwyt na wątek
  pthread_t thread1;
  struct thread_data_t t_data;
  t_data.msgid = msgid;
  t_data.semid = semid;
  printf("t_data.semid = %d\n", t_data.semid);
  int create_result = pthread_create(&thread1, NULL, matchClients, (void *)&t_data);
  if (create_result){
     printf("Błąd przy próbie utworzenia wątku dobierajacego w pary klientów, kod błędu: %d\n", create_result);
     exit(-1);
  }
  sleep(1);//wait a second to allow matchClients to save sent data
}


void acceptClients(int server_sct_dsc, int semid, int msgid){
  struct msgbuf sender;
  sender.mtype = 1;
  int msgSucces;
  int conn_sct_dsc = -1;

  conn_sct_dsc = accept(server_sct_dsc, NULL, NULL);
  printf("połączono\n");
  if (conn_sct_dsc < 0)
  {
     fprintf(stderr, "Błąd przy próbie utworzenia gniazda dla połączenia.\n");
     close(server_sct_dsc);
     //releaseIPC(semid, shmid, msgid);
     exit(1);
  }
  sender.conn_sct_dsc = conn_sct_dsc;

  msgSucces = msgsnd(msgid, &sender, sizeof(sender.conn_sct_dsc), 0);
  if ( msgSucces == -1){
    printf("Argumenty msgid=%d\nsender.conn_sct_dsc=%d\n", msgid, sender.conn_sct_dsc);
    perror("Błąd przy próbie wysłania komunikatu");
    close(server_sct_dsc);
    //releaseIPC(semid, shmid, msgid);
    exit(1);
  }
  else{
    mutex_unlock(semid, 0, 1);//allow matchClients function to take message
  }
}


int main(int argc, char* argv[])
{
  int semid, shmid, msgid;

   int server_sct_dsc;


   server_sct_dsc = prepare_socket(argv);
   createIPC(&semid, &shmid, &msgid);
   printf("%d\n", semid);
   wakeUpMatchClients(semid, msgid);
   while(1)
   {
    acceptClients(server_sct_dsc, semid, msgid);
  }
  releaseIPC(semid, shmid, msgid);
   close(server_sct_dsc);
   return(0);
}
