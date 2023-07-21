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
#include <pthread.h>
#include <sys/select.h>
#define MAXLINE 4096
// variabili globali 
int TOs = 0; 
int TOms = 0;
int seqnum=0;
int sockfd;
int lt_ack_rcvd=0;
int cwnd=0;
int CongWin=1;
double p=0;
int maxackrcv=-1;
int dim=200;
//preforking variable 
static int nchildren;
static pid_t *pids;
static struct flock lock_it, unlock_it;
static int lock_fd = -1;
/* fcntl() will fail if my_lock_init() not called */

// globali
struct sockaddr_in addr;
socklen_t addrlen = sizeof(struct sockaddr_in);

// implementa il controllo di segnali per gestire poi i figli
typedef void Sigfunc(int);

//pkt struct 
struct st_pkt
	{
		int ack;
    int finbit;
		char pl[MAXLINE];
    
	};
//start dimensione 20
struct st_pkt *retr;

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
//thread per la gestione del ack cum ecc
void * rcv_cong(){
struct st_pkt pkt;
int k = 0,msgRitr=0;
// Set up the struct timeval for the timeout.
fd_set fds;
int n;
struct timeval tv;

// Set up the file descriptor set.
FD_ZERO(&fds);
FD_SET(sockfd, &fds);

//implementare un TO dinamico 

tv.tv_usec = TOms; //ms waitingee
tv.tv_sec = TOs; //s waiting

// Wait until timeout or data received.
bool stay=true;
while(stay){
 n = select (sizeof(fds)*8,&fds, NULL, NULL, &tv);
  if ( n == 0)
  { 
   //printf("\nTimeout ritrasmetto i pkt\n");
     CongWin= CongWin >> 1; 
   fflush(stdout);
   //perchè gli ack che ricevo corrispondono all pkt che il client si aspetta di ricevere 
   k=lt_ack_rcvd;
    //implemento la ritrasmissione di tutti i pkt dopo lt_ack_rcvd
    //ritrasmetto se ultimo ack è < del mio numero 
    while( k < seqnum ){
				  if ((sendto(sockfd,&retr[k],sizeof(pkt), 0, (struct sockaddr *)&addr,addrlen)) < 0)  {
					  perror("errore in sendto");
              	  			exit(1);
				  }
          msgRitr++;
				  k++;
          k = k % dim;
				  }
  }
  else if( n == -1 )
  {
    perror("Error in select wait");
    exit(1);
  }
//se non ho errore allora leggo e vedo l ack che il receiver mi invia
  if (recvfrom(sockfd, &pkt, sizeof(pkt),0,(struct sockaddr *)&addr, &addrlen ) <0 ){
       			 perror("errore in recvfrom");
        		 exit(1);
  }
  //ultimo ack ricevuto(ricevo ack comulativi)
  lt_ack_rcvd=pkt.ack;
  printf("\nlt_ack %d   seqnum %d\n",pkt.ack,seqnum);
  fflush(stdout);
  //printf("\nServer : Ack cum  %d  seqnum att %d \n",pkt.ack,seqnum);
  //fflush(stdout);
  //incremento la finestra di trasmissione  ogni ack se arriva un ack nuovo 
   if(pkt.finbit == 1 && pkt.ack == seqnum){
    printf("Server : Client disconesso  correttamente \n");
    fflush(stdout);
    stay = false;
    continue;
  }
  if(maxackrcv < lt_ack_rcvd){
    CongWin=CongWin << 1;
    //printf("\n CongWin dim %d\n",CongWin);
    //fflush(stdout);
  }
  //mantengo il num di ack più alto ricevuto 
  if(maxackrcv < lt_ack_rcvd){
    maxackrcv=lt_ack_rcvd;
  }
}
printf("\nMSG Ritr %d\n",msgRitr);
}
// gestice nello specifico il comando get
void send_get(char *str) {
  if((retr=malloc(sizeof(struct st_pkt)*dim)) == NULL){
    perror("Error malloc");
    exit(1);
  }
	struct st_pkt pkt;
  FILE *file;
  bool stay=true;
  int i=0,msgInviati=0,msgPerso=0,msgTot=0,dimpl=0; 
  double prob=0;
  char path_file[MAXLINE];
  sprintf(path_file,"Server_Files/%s",str);
  if((file = fopen(path_file, "r+")) == NULL){
    printf("Errore in open del file\n");
    exit(1);
  }
   pthread_t thread_id;
  // creo il thread che mi legge le socket (lettura bloccante)
  if(pthread_create(&thread_id,NULL,rcv_cong,NULL) != 0){
    perror("error pthread_create");
    exit(1);
  }

  while(stay){
    //se last ack non è uguale al mio seqnum mi fermo altrimenti entro dentro e invio da 0 a CongWin pkt e poi mi aspetto di ricevere come lastack quello dell'ultimo pkt inviato poi continuo 
    if(lt_ack_rcvd == seqnum){ 
    cwnd=0; 
    while(cwnd < CongWin && stay == true){
      cwnd++;
      while((dimpl=fread(pkt.pl,1,sizeof(pkt.pl),file)) == MAXLINE){
        seqnum++;
        pkt.finbit=0;
        pkt.ack=seqnum;
        //mantengo CongWin pkt
        retr[i]=pkt;
        //printf("\nACK RETR %d  I= %d\n",retr[i].ack,i);
        fflush(stdout);
        //printf("\n i VALUE %d\n",i);
        //printf("\n retr[i] num %d\n",retr[i].ack);
        //fflush(stdout);
		    i++;
        //cicliclo 
		    i = i % dim; 
        //printf("Server : Payload:  %s\n",pkt.pl);
		    fflush(stdout);

        //aumento la dim del vettore che mi salva i pkt 
        /*
        if(seqnum == dim){
          //raddopio la dimensione di retr
          dim=dim<<1;
          retr=realloc(retr,dim*sizeof(struct st_pkt));
          if (retr == NULL){
            perror("Realloc could not allocate new memory");
            exit(1);
          }
          printf("\nDIM VALUE %d\n",dim);
        }
        */
        prob=(double)rand() / RAND_MAX;
        if(prob < p){
          //printf("Server : pkt lost  %d\n",pkt.ack);
          //fflush(stdout);
            msgPerso++;
            msgTot++;
          // Il messaggio è stato perso 
          continue;
        }else{
          //printf("Server : send pkt %d\n",pkt.ack);
          // Trasmetto con successo 
		if ((sendto(sockfd,&pkt,sizeof(pkt),0,(struct sockaddr *)&addr,addrlen)) < 0)  
		{	
			perror("errore in sendto");
              		exit(1);
		}		
    msgInviati++;
    msgTot++;
        }
    //la lettura la fa il thread cosi non mi blocco io main thread 
	}

  //fine lettura del file  
	if(feof(file)){
    while(lt_ack_rcvd != seqnum){
      printf("OOOOOO\n");
    }
    seqnum++;
		pkt.ack=seqnum;
    pkt.finbit=1;
    pkt.pl[dimpl]='\0';
		retr[i]=pkt;
    prob=(double)rand() / RAND_MAX;
        if(prob < p){
          //printf("\n Pkt lost %d\n",pkt.ack);
          //fflush(stdout);
          msgPerso++;
          // Il messaggio è stato perso 
          msgTot++;
          continue;
        }else{
          printf("Server : send pkt %d\n",pkt.ack);
          printf("\n Server : Close connection \n");
          fflush(stdout);
          stay=false;
	//invio il terminatore 
	if (sendto(sockfd,&pkt,sizeof(pkt), 0, (struct sockaddr *)&addr, addrlen ) <0 ) {
       		 perror("errore in recvfrom");
        	 exit(1);
		}
    msgInviati++;
    msgTot++;
  }
  }
  }
  }
  }
  //aspetto la terminazione del thread che legge 
  if(pthread_join(thread_id,NULL) != 0 ){
    perror("Error pthread_join");
    exit(1);
  }
  printf("\nMSG TOTALI %d\n",msgTot);
  printf("\nMSG PERSI %d\n",msgPerso);
  printf("\nMSG INVIATI %d\n",msgInviati);
  printf("\n Dim CongWin finale %d\n",CongWin);
  printf("\n PROB DI SCARTARE UN MSG %f\n",p);
  fflush(stdout);
}

// gestice nello specifico il comando put
void rcv_put(int sockfd,char *file) {
	char path_file[200];

	printf("\nSend_put alive\n");
	printf("ecco il nome del file %s\n",file);
	sprintf(path_file,"Server_Files/%s",file);

	FILE * fptr;
	//creo il file se già esiste lo cancello tanto voglio quello aggiornato
	
	char rcv_buff[4098];
	char snd_buff[30];
	char number_pkt[30];
	int n=1,i=0,k=1;
	bool stay=true;
	if((fptr = fopen(path_file,"r+")) == NULL){
		perror("Error opening file");
		exit(1);
		}
	while(stay){

		snd_buff[0]='\0';
		number_pkt[0]='\0';
		rcv_buff[0]='\0';

		if ((k=recvfrom(sockfd, rcv_buff, 4098, 0, (struct sockaddr *)&addr, &addrlen ))< 0) {
        		perror("errore in recvfrom");
       			exit(1);
		}

		while(rcv_buff[i] != '\n'){
			i++;
		}
		rcv_buff[k-1]='\0';

		strncpy(number_pkt,rcv_buff,i);

		number_pkt[i]='\0';

	// END == terminatore pkt inviati
	
	printf("\n NUM RICEVUTO-> %s\n",number_pkt);

	printf("\n NUM CHE VOGLIO->%d\n",n);



	fflush(stdout);

        if(!strcmp(number_pkt,"END")){

		printf("Terminatore ricevuto\n");

		//devo aprire il file e scriverci l'ultimo pkt
		printf("\nPKT-->%s\n",rcv_buff);

		rcv_buff[MAXLINE+2]='\0';

		if((fwrite(rcv_buff+i,strlen(rcv_buff)-i,1,fptr) <0 )){
				perror("Error in write rcv_get\n");
				exit(1);
				}
		stay=false;

		//invio  ACK di fine ricezione al sender 
		sprintf(snd_buff,"OK\n");


		printf("invio il pkt %s\n",snd_buff);

		fflush(stdout);

		if (sendto(sockfd, snd_buff, strlen(snd_buff), 0,(struct sockaddr *)&addr, addrlen) < 0) {
        			perror("errore in sendto");
        			exit(1);
		}
		fclose(fptr);
	//confronto il numero ricevuto e quello che mi aspetto
        }else if(n==strtol(number_pkt,NULL,10) && stay == true ){

		printf("\nPKT-->%s\n",rcv_buff);

		if((fwrite(rcv_buff+i,strlen(rcv_buff)-i,1,fptr) <0 )){
				perror("Error in write rcv_get\n");
				exit(1);
				}
		//incremento n
		n++;
		}
	else if( n != strtol(number_pkt,NULL,10) && stay == true ){
		//gestire ack non in ordine ES: inviamo un ack al sender e gli diciamo di inviare tutto dopo quel numero
		printf("Numero ricevuto diverso da quello che mi aspettavo\n");
		// invio al sender un ack comulativo
		sprintf(snd_buff,"%d\n",n);

		if (sendto(sockfd, snd_buff, strlen(snd_buff), 0,(struct sockaddr *)&addr, addrlen) < 0) {
        		perror("errore in sendto");
        			exit(1);
		}
	}
    }
	printf("\nreturn rcv_put\n");
	fflush(stdout);
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
		  strncpy(fin_buff+strlen(fin_buff),snd_buff+2,strlen(snd_buff)+2);
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
  int i=0;
  bool stay=true;
  char cd[100];
  char name[100];
  struct st_pkt pkt;

  if ((recvfrom(sockfd, &pkt,sizeof(pkt),0, (struct sockaddr *)&addr,&addrlen)) < 0) {
      perror("errore in recvfrom");
      exit(-1);
    }
    while(pkt.pl[i] != ' '){
      i++;
    }
    strncpy(cd,pkt.pl,i);
    // +1 perchè non voglio lo spazio 
    strcpy(name,pkt.pl+i+1);

    printf(" Server : Msg ricevuto   %s\n", pkt.pl); 

/* ---------------------- gestisco caso get ----------------------------------------------*/

    if(!strcmp("get",cd)){
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
    // Leggi i nomi dei file nella cartella
    while ((entry = readdir(dir)) != NULL && !found) {
        char fullPath[500];  // Percorso completo del file
        snprintf(fullPath, sizeof(fullPath), "%s/%s", "Server_Files", entry->d_name);
        if (stat(fullPath, &fileStat) == 0 && S_ISREG(fileStat.st_mode) && !strcmp(entry->d_name, name)) {           
            found = true;
        }
    }
    if(found){
      pkt.ack=0;
    strcpy(pkt.pl,"File trovato.");
    printf("\nServer : %s\n",pkt.pl);
    if ((sendto(sockfd, &pkt, sizeof(pkt), 0, (struct sockaddr *)&addr,addrlen)) < 0)  {
                perror("errore in sendto");
                exit(1);
    }
    // entro nella funzione che implementa la send del file 
     send_get(name);
    }else if(!found){
    //Gestisco caso file non trovato
    strcpy(pkt.pl,"File non trovato, riprova.");
    pkt.ack=-1;
    printf("invio ack con errore[%s]",pkt.pl);

    if ((sendto(sockfd, &pkt, sizeof(pkt), 0, (struct sockaddr *)&addr,addrlen)) < 0)  {
                perror("errore in sendto");
                exit(1);
            }
    }
    // Chiudi la cartella
    closedir(dir); 
    //continue;
    }

/* ---------------------------------- gestisco caso list------------------------------------------------*/


    if(!strcmp("list",cd)){
    strcpy(pkt.pl,"List in esecuzione \n");
    pkt.ack=0;
    if ((sendto(sockfd, &pkt, sizeof(pkt), 0, (struct sockaddr *)&addr,addrlen)) < 0)  {
                perror("errore in sendto");
                exit(1);
            }
	 send_list(sockfd);	 
    }

/* ------------------------ gestisco caso put --------------------------------------*/
    if(!strcmp("put",cd)){
	 printf("\nStart command put\n");
      strcpy(pkt.pl,"Put  in esecuzione");
      if ((sendto(sockfd, &pkt, sizeof(pkt), 0, (struct sockaddr *)&addr,addrlen)) < 0)  {
        perror("errore in sendto");
        exit(1);
      }
      rcv_put(sockfd,name);
    }
}


void child_main(int i, int listenfd, int addrlen) {

  printf("\n child %ld starting\n", (long)getpid());

  for (;;) {
    my_lock_wait(); /* my_lock_wait() usa fcntl() */
    send_control(listenfd); /* processa la richiesta */
    my_lock_release();
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
  if (argc != 5) {
    fprintf(stderr, " utilizzo:<pkt loss probab P , TO[s] , TO[ms] , SERV_Port > \n");
    exit(1);
  }
  // variabili richieste dalla traccia
  p=strtod(argv[1],NULL);
  TOs = atoi(argv[2]);
  TOms = atoi(argv[3]);
  int SERV_PORT = atoi(argv[4]);

  printf("\nProb inserita %f\n",p);
  printf("\nTO inserito %d [s] \n",TOs);
  printf("\nTO inserito %d [ms] \n",TOms);
  printf("\nSERV_PORT inserito %d\n",SERV_PORT);

  if (SERV_PORT < 255) {
    fprintf(stderr, "inserire num. porta dedicato inserire un numero > 255");
    exit(1);
  }
  // variabili per il prefork
  int listenfd, i;
  // queste due nemmeno servono
  void sig_int(int);

  // variabili normali
  nchildren = 5;  // numeri di figli qui possiamo rendere le cose dinamiche

  if ((listenfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) { /* crea il socket */
    perror("errore in socket");
    exit(1);
  }
  sockfd=listenfd;
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