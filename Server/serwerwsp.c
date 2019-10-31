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
};


static struct sembuf buf2;

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
        exit(1);
    }
}


/*
semid - int sent to function to receive
id of semaphores
*/
void createIPC(int *semid, int *shmid){
	//creating IPC objects
  *semid = semget(KEY, MAX_GAMES, IPC_CREAT|0600);
  if(*semid == -1){
    perror("Blad przy utworzeniu zbioru semaforow\n");
		exit(1);
	}
  //shared memory keep conn_sct for all clients, who wait for game
  *shmid = shmget(KEY, (MAX_GAMES * 2 + 1) *sizeof(int), IPC_CREAT|0600);

  if(*shmid == -1){
        perror("Blad przy utworzeniu pamięci współdzielonej \nByć może taki klucz juz istnieje dla pamieci wspoldzielonej\n");
  }
  for(int i = 0; i < MAX_GAMES; i++){
    if(semctl( *semid, 0, SETVAL, 1) == -1){
  		perror("Nadanie wartosci semaforowi nr " + i);
  		exit(1);
  	}
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
void *ThreadBehavior(void *t_data)
{
  int detach_succes = pthread_detach(pthread_self());
  if(detach_succes != 0){
    fprintf(stderr, "Can not detach thread.\n");
    exit(-1);
  }
  struct thread_data_t *th_data = (struct thread_data_t*)t_data;
  //dostęp do pól struktury: (*th_data).pole
  //TODO (przy zadaniu 1) klawiatura -> wysyłanie albo odbieranie -> wyświetlanie

  char buf[100];
  while(1)
  {
    for(int i=0;i<100;i++)
    	buf[i]=' ';
    fgets(buf,sizeof(buf), stdin);
    for(int i=0;i<3;i++)
      write(th_data->conn_sct_dsc, buf, sizeof(buf));
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

void releaseIPC(int semid, int shmid){
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
}

int main(int argc, char* argv[])
{
   int server_sct_dsc;
   int conn_sct_dsc = -1;
   server_sct_dsc = prepare_socket(argv);
   while(1)
   {
    //for(int i=0;i<3;i++){
    conn_sct_dsc = accept(server_sct_dsc, NULL, NULL);//tab[i]
    printf("połączono\n");
    if (conn_sct_dsc < 0)
    {
       fprintf(stderr, "%s: Błąd przy próbie utworzenia gniazda dla połączenia.\n", argv[0]);
       close(server_sct_dsc);
       exit(1);
    }
    //}
    handleConnection(conn_sct_dsc);
  }

   close(server_sct_dsc);
   return(0);
}
