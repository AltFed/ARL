#include <arpa/inet.h>
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
  FILE *file;
  char fin_buff[MAXLINE+10];	// poi modificare il 10 deve essere comunque maggiore di MAXLINE di almeno 3/4 
  char snd_buff[MAXLINE+10];
  char rcv_buff[100];
  char path_file[200];
  char *fin="END\n";
  bool stay=true;
  bool error=true;
  int temp=1;
  int rcv_number=0;
  sprintf(path_file,"Server_Files/%s",str);
  printf("path %s\n",path_file);
  if((file=fopen(path_file, "r+")) == NULL){
    printf("Errore in open del file\n");
    exit(1);
  }
  while(stay){
  fin_buff[0]='\0';	  
  snd_buff[0]='\0';
  rcv_buff[0]='\0';
  // inserisco il numero al pkt che invio 
  sprintf(snd_buff,"%d\n",temp);
  //leggo il file 
  if((fread(snd_buff+2,MAXLINE,1,file) < 0)){
		  perror("Error read file\n");
		  exit(1);
		  }
  printf("pkt che invio al recever %s\n",snd_buff);
  if(strlen(snd_buff) < MAXLINE){	
	 printf("Chiusura comando list\n");
	sprintf(fin_buff,"%s",fin);
	 strncpy(fin_buff+strlen(fin_buff),snd_buff+2,strlen(snd_buff)-2);
	// +1: perchè tolgo il numero 
        // -1: perchè copio i bit ma devo togliere il numero  che vengono contati dallo strlen
	if ((sendto(sockfd,fin_buff ,strlen(fin_buff), 0, (struct sockaddr *)&addr,addrlen)) < 0)  {
               perror("errore in sendto");
               exit(1);
	}
	stay=false;
	 }else{
		 // controllo se è arrivato un ack dal receiver in caso gestisco gli errori non bloccante
		 /*if ((recvfrom(sockfd,rcv_buff,sizeof(rcv_buff),MSG_DONTWAIT, (struct sockaddr *)&addr,&addrlen)) < 0) {
      					perror("errore in recvfrom");
      					exit(-1);
		 }
		 //ricevo qualcosa ancora da implementare 
		 if(strlen(rcv_buff) != 0){
			 // se il numero ricevuto è diverso da quello corrente errore il receiver non bufferizza nulla  
			 rcv_number=strtol(rcv_buff,NULL,10);
			 if(rcv_number != temp ){
				 while(error){

					 // qui devo implementare una variabile globale che mantiene i buff inviati almeno un po
					 // poi invio tutti i pkt dal rcv_number fino al temp e esco
					 error=false;
				 }
			 }
		 }
*/
		//invio il pkt 
		 if ((sendto(sockfd, snd_buff,strlen(snd_buff), 0, (struct sockaddr *)&addr,addrlen)) < 0)  {
                			perror("errore in sendto");
                			exit(1);
		 }
		 temp++;
	 }
  }
  printf("send_get return\n");
}

// gestice nello specifico il comando put
void rcv_put(int sockfd,char *str) {

	printf("send_put alive\n");
}

// gestice nello specifico il comando list
void send_list(int sockfd) {
  printf("send_list alive\n");
  char fin_buff[MAXLINE+10];	// poi modificare il 10 deve essere comunque maggiore di MAXLINE di almeno 3/4 
  char snd_buff[MAXLINE+10];
  char rcv_buff[100];
  char *fin="END\n";
  DIR *directory;
  struct dirent *file;
  bool stay=true;
  bool error=true;
  int temp=1;
  int rcv_number=0;
  while(stay){
  fin_buff[0]='\0';	  
  snd_buff[0]='\0';
  rcv_buff[0]='\0';
  sprintf(snd_buff,"%d\n",temp);
  // Apertura della cartella
  directory = opendir("Server_Files");

  // Verifica se la cartella è stata aperta correttamente
  if (directory == NULL) {
    printf("Impossibile aprire la cartella.\n");
    exit(1);
  }

    // Lettura dei file all'interno della cartella
    while ((file = readdir(directory)) != NULL) {
        // Ignora le voci "." e ".."
        if (strcmp(file->d_name, ".") != 0 && strcmp(file->d_name, "..") != 0) {

		// implementare controllo sulla lunghezza attuale della snd_buff 

            sprintf(snd_buff+strlen(snd_buff), "%s\n", file->d_name);
	}
    }
       	//qui vedo se il buff che invio al client è pieno o meno se si vado avanti altrimenti inserisco il terminatore
	 if(strlen(snd_buff) < MAXLINE){
		 printf("snd_buff %s",snd_buff);
		 printf("Chiusura comando list\n");
		  sprintf(fin_buff,"%s",fin);
		  strncpy(fin_buff+strlen(fin_buff),snd_buff+2,strlen(snd_buff)-2);
		  // +1: perchè tolgo il numero 
		  // -1: perchè copio i bit ma devo togliere il numero  che vengono contati dallo strlen
	          if ((sendto(sockfd,fin_buff ,strlen(fin_buff), 0, (struct sockaddr *)&addr,addrlen)) < 0)  {
               		perror("errore in sendto");
                	exit(1);
		  }
		  stay=false;
	 }else{
		 // controllo se è arrivato un ack dal receiver in caso gestisco gli errori non bloccante
		 if ((recvfrom(sockfd,rcv_buff,sizeof(rcv_buff),MSG_DONTWAIT, (struct sockaddr *)&addr,&addrlen)) < 0) {
      					perror("errore in recvfrom");
      					exit(-1);
		 }
		 //ricevo qualcosa
		 if(strlen(rcv_buff) != 0){
			 // se il numero ricevuto è diverso da quello corrente errore il receiver non bufferizza nulla  
			 rcv_number=strtol(rcv_buff,NULL,10);
			 if(rcv_number != temp ){
				 while(error){

					 // qui devo implementare una variabile globale che mantiene i buff inviati almeno un po
					 // poi invio tutti i pkt dal rcv_number fino al temp e esco
					 error=false;
				 }
			 }
		 }
		//invio il pkt 
		 if ((sendto(sockfd, snd_buff,strlen(snd_buff), 0, (struct sockaddr *)&addr,addrlen)) < 0)  {
                			perror("errore in sendto");
                			exit(1);
		 }
		 temp++;
	 }
  }
  printf("chiudo la directory\n");
	// Chiusura della cartella
    	closedir(directory);
}
// gestisce il comando che il client richiede
void send_control(int sockfd) {
  char *buff = malloc(MAXLINE);
  char *str = malloc(MAXLINE);
  int i = 0, k = 0;
   char ack[100];
    while (1) {
	  buff[0]='\0';
	  str[0]='\0';
	  i=0;
	  k=0;
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
    


/* ---------------------- gestisco caso get ----------------------------------------------*/

    if(!strncmp("get", buff+i+1,3)){
    bool found = false;
    DIR *dir;
    struct dirent *entry;
    struct stat fileStat;
      // Apri la cartella
    dir = opendir("Server_Files");
    if (dir == NULL) {
        perror("Impossibile aprire la cartella");
        exit(1);
    }
    printf("CIAO %s\n",buff+i+5);
    // Leggi i nomi dei file nella cartella
    while ((entry = readdir(dir)) != NULL && !found) {
        char fullPath[500];  // Percorso completo del file
        snprintf(fullPath, sizeof(fullPath), "%s/%s", "Server_Files", entry->d_name);
        if (stat(fullPath, &fileStat) == 0 && S_ISREG(fileStat.st_mode) && !strcmp(entry->d_name, buff+i+5)) {           
            found = true;
        }
    }
    if(found){
    puts("File trovato mando ack e entro in send_get");
    sprintf(ack+i+1,"3\nFile trovato. \n");
    printf("Invio ACK %s\n",ack);
    if ((sendto(sockfd, ack ,strlen(ack), 0, (struct sockaddr *)&addr,addrlen)) < 0)  {
                perror("errore in sendto");
                exit(1);
    }

    // entro nella funzione che implementa la send del file 
    
     send_get(sockfd,buff+i+5);


    }else if(!found){

    //Gestisco caso file non trovato
    
    sprintf(ack+i+1,"-1\n File non trovato, riprova.\n");
    puts("invio ack con errore");
    if ((sendto(sockfd, ack ,strlen(ack), 0, (struct sockaddr *)&addr,addrlen)) < 0)  {
                perror("errore in sendto");
                exit(1);
            }
    }
    // Chiudi la cartella
    closedir(dir); 
    continue;
    }

/* ---------------------------------- gestisco caso list ------------------------------------------------*/


    if(!strncmp("list", buff+i+1, 4)){
    sprintf(ack+i+1,"3\nList in esecuzione \n");
    printf("Invio ACK->%s\n",ack);
    if ((sendto(sockfd, ack,strlen(ack), 0, (struct sockaddr *)&addr,addrlen)) < 0)  {
                perror("errore in sendto");
                exit(1);
            }
	 send_list(sockfd);	 
    }

/* ------------------------ gestisco caso put --------------------------------------*/
    if(!strncmp("put", buff+i+1,  3)){
      puts("invio ack, creo il file e chiamo rcv_put");
      puts("File trovato mando ack e entro in send_get");
      sprintf(ack+i+1,"3\nFile trovato. \n");
      puts("Invio ACK");
      if ((sendto(sockfd, ack ,strlen(ack), 0, (struct sockaddr *)&addr,addrlen)) < 0)  {
        perror("errore in sendto");
        exit(1);
    }

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
