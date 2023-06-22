#include<arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <wait.h>
#include <stdbool.h>
#include <time.h>
#include <dirent.h>
#define L 4
#define MAXLINE 4096
// vedere gli indirizzi da usare per recvform sendto
int TO = 0;
static int nchildren;
static pid_t *pids;
static struct flock lock_it, unlock_it;
static int lock_fd = -1;
/* fcntl() will fail if my_lock_init() not called */

// globali cosi posso usare più funzioni per rcvform e sendto
//
struct sockaddr_in addr;
socklen_t addrlen = sizeof(struct sockaddr_in);

// implementa il controllo di segnali per gestire poi i figli
typedef void Sigfunc(int);
Sigfunc *signal(int signum, Sigfunc *handler);

// implementa la logica di lock/unlock
void my_lock_init(char *pathname) {
  char lock_file[1024];

  /* must copy caller's string, in case it's a constant */
  strncpy(lock_file, pathname, sizeof(lock_file));
  if ((lock_fd = mkstemp(lock_file)) < 0) {
    fprintf(stderr, "errore in mkstemp");
    exit(1);
  }

  if (unlink(lock_file) == -1) { /* but lock_fd remains open */
    fprintf(stderr, "errore in unlink per %s", lock_file);
    exit(1);
  }
  lock_it.l_type = F_WRLCK;
  lock_it.l_whence = SEEK_SET;
  lock_it.l_start = 0;
  lock_it.l_len = 0;

  unlock_it.l_type = F_UNLCK;
  unlock_it.l_whence = SEEK_SET;
  unlock_it.l_start = 0;
  unlock_it.l_len = 0;
}

void my_lock_wait() {
  int rc;

  while ((rc = fcntl(lock_fd, F_SETLKW, &lock_it)) < 0) {
    if (errno == EINTR)
      continue;
    else {
      fprintf(stderr, "errore fcntl in my_lock_wait");
      exit(1);
    }
  }
}

void my_lock_release() {
  if (fcntl(lock_fd, F_SETLKW, &unlock_it) < 0) {
    fprintf(stderr, "errore fcntl in my_lock_release");
    exit(1);
  }
}

// gestice nello specifico il comando get
void send_get(int sockfd,char *str) {

	puts("Hello, send_get here");


}

// gestice nello specifico il comando put
void send_put(int sockfd,char *str) {

	printf("send_put alive\n");
}

// gestice nello specifico il comando list
void send_list(int sockfd,char * str) {
	DIR * d;
	struct dirent *dir;
	int k=1;
	char *snd_buff=malloc(MAXLINE);
	printf("send_list alive\n");
	d=opendir("prova");
	if(dir){
		while((dir = readdir(d)) != NULL ){
			// qui aggiungere ack ecc 
			snd_buff[0]='\0';
			sprintf(snd_buff,"%d ",k);
			k++;
			strcat(snd_buff,dir->d_name);
			printf(" send_list invio msg %s\n",snd_buff);

			if ((sendto(sockfd, snd_buff  ,strlen(snd_buff), 0, (struct sockaddr *)&addr,addrlen)) < 0)  {
      				perror("errore in sendto");
     				 exit(-1);
		       	}
		}

		closedir(d);
	}
}

// gestisce il comando che il client richiede
void send_control(int sockfd) {
  char *buff = malloc(MAXLINE);
  char *str = malloc(MAXLINE);
  int lenbuff = 0;
  int i = 0, k = 0;
  char ack[100];
  char seq[100];
  while (1) {
	  buff[0]='\0';
	  str[0]='\0';
    // ho capito se leggiamo e poi implementiamo la gestione delle cose su altre
    // funzione allora dobbiamo passare anche il buffer sennò quello che leggo
    // cancello dalla socket vedere anche il client in tale caso !!!!!!!
    if ((recvfrom(sockfd, buff,MAXLINE,0, (struct sockaddr *)&addr,&addrlen)) < 0) {
      perror("errore in recvfrom");
      exit(-1);
    }
    printf(" rcv_buff -->%s\n",buff);
    while (buff[i] != '\n') {
      i++;
    }

    strncpy(ack, buff, i);
    ack[i] = '\n';
    ack[i+1] = '\0';


    printf("seq ricevuto %s --> invio ack %s\n", ack, ack);
    printf("msg ricevuto %s\n", buff+i+1);
    
    if(!strncmp("get", buff+i+1,3)){
    bool found = false;
    DIR *dir;
    struct dirent *entry;
    struct stat fileStat;

    // Apri la cartella
    dir = opendir("Server_Files");
    if (dir == NULL) {
        perror("Impossibile aprire la cartella");
        exit(-1);
    }
    // Leggi i nomi dei file nella cartella
    while ((entry = readdir(dir)) != NULL && !found) {
        char fullPath[256];  // Percorso completo del file
        snprintf(fullPath, sizeof(fullPath), "%s/%s", "Server_Files", entry->d_name);
        if (stat(fullPath, &fileStat) == 0 && S_ISREG(fileStat.st_mode) && !strcmp(entry->d_name, buff+i+5)) {           
            found = true;
        }
    }
    if(found){
      puts("File trovato mando ack e entro in send_get");
      send_get(sockfd ,entry->d_name);
      printf(" invio ack:%s\n", ack);
      if ((sendto(sockfd, ack ,MAXLINE, 0, (struct sockaddr *)&addr,addrlen)) < 0)  {
          perror("errore in sendto");
          exit(-1);
      }
     // send_get(sockfd, entry->d_name);
    }else if(!found){
    //Gestisco caso file non trovato
    
    sprintf(ack+i+1,"-1\n File non trovato, riprova. \n");
    puts("invio ack con errore");
    if ((sendto(sockfd, ack ,MAXLINE, 0, (struct sockaddr *)&addr,addrlen)) < 0)  {
                perror("errore in sendto");
                exit(-1);
            }
    }
    // Chiudi la cartella
    closedir(dir); 
    continue;
    }




    if(!strncmp("list", buff+i+1, 4)){
        puts("invio ack, creo il file e chiamo send_list");
    }
    if(!strncmp("put", buff+i+1,  3)){
        puts("invio ack, creo il file e chiamo rcv_put");
    }

    if ((sendto(sockfd, ack ,MAXLINE, 0, (struct sockaddr *)&addr,addrlen)) < 0)  {
      perror("errore in sendto");
      exit(-1);
    }

  }
}

void child_main(int i, int listenfd, int addrlen) {
  printf("child %ld starting\n", (long)getpid());

  for (;;) {
    my_lock_wait(); /* my_lock_wait() usa fcntl() */
    my_lock_release();
    // va bene cosi perchè sennò lavora solo una per volta invece cosi solo uno
    // per volta legge il comando
    send_control(listenfd); /* processa la richiesta */
  }
}

// creo i processi figli
pid_t child_make(int i, int listenfd, int addrlen) {
  pid_t pid;
  if ((pid = fork()) > 0)
    return (pid);                    // processo padre
                                     //
  child_main(i, listenfd, addrlen);  // non ritorna mai
}

// blocco i segnali esterni tranne alcuni che decido ctrl
Sigfunc *signal(int signum, Sigfunc *func) {
  struct sigaction act, oact;
  /* la struttura sigaction memorizza informazioni riguardanti la
  manipolazione del segnale */

  act.sa_handler = func;
  sigemptyset(&act.sa_mask); /* non occorre bloccare nessun altro segnale */
  act.sa_flags = 0;
  if (signum != SIGALRM) act.sa_flags |= SA_RESTART;
  if (sigaction(signum, &act, &oact) < 0) return (SIG_ERR);
  return (oact.sa_handler);
}

// funzione che gestisce il timeout
void sig_time(int signo) {}

// termina i figli non ho stato di zombie
void sig_int(int signo) {
  int i;
  /* terminate all children */
  for (i = 0; i < nchildren; i++) kill(pids[i], SIGTERM);
  while (wait(NULL) > 0)
    ; /* wait for all children */

  if (errno != ECHILD) {
    fprintf(stderr, "errore in wait");
    exit(1);
  }
  exit(0);
}

int main(int argc, char **argv) {
  if (argc != 4) {
    fprintf(stderr, " utilizzo:<variabile P, TO , SERV_Port > \n");
    exit(1);
  }
  // variabili richieste dalla traccia
  int p = atoi(argv[1]);
  TO = atoi(argv[2]);
  int SERV_PORT = atoi(argv[3]);
  srand (time(NULL));
  if (SERV_PORT < 255) {
    fprintf(stderr, "inserire num. porta >255");
    exit(1);
  }
  // variabili per il prefork
  int listenfd, i;
  // queste due nemmeno servono
  void sig_int(int);

  // variabili normali
  nchildren = 5;  // numeri di figli qui possiamo rendere le cose dinamiche
                  //
  if ((listenfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) { /* crea il socket */
    perror("errore in socket");
    exit(1);
  }

  memset((void *)&addr, 0, sizeof(addr));
  addr.sin_family = AF_INET;
  addr.sin_addr.s_addr =
      htonl(INADDR_ANY); /* il server accetta pacchetti su una qualunque delle
                            sue interfacce di rete */
  addr.sin_port = htons(SERV_PORT); /* numero di porta del server */

  /* assegna l'indirizzo al socket */
  if (bind(listenfd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
    perror("errore in bind");
    exit(1);
  }
  // implemento la logica della prefork con file locking

  // qui alloco un area di memoria per mantenere i pid dei child
  pids = (pid_t *)calloc(nchildren, sizeof(pid_t));
  if (pids == NULL) {
    fprintf(stderr, "errore in calloc");
    exit(1);
  }

  my_lock_init(("/tmp/lock.XXXXXX")); /* one lock file for all children */
  for (i = 0; i < nchildren; i++) {
    // inserisco nell array pids i pid dei figli che mi ritornano dalla
    // child_make
    pids[i] = child_make(i, listenfd, addrlen); /* parent returns */
  }
  if (signal(SIGINT, sig_int) == SIG_ERR) {
    fprintf(stderr, "errore in signal INT ");
    exit(1);
  }
  if (signal(SIGALRM, sig_time) == SIG_ERR) {
    fprintf(stderr, "errore in signal TO");
    exit(1);
  }
  system("mkdir Server_Files");
  for (;;) pause(); /* everything done by children */
}
