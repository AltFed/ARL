#include <sys/types.h> 
#include <sys/socket.h> 
#include <arpa/inet.h>
#include <unistd.h> 
#include <stdio.h> 
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <signal.h>
#include <errno.h>
#include <wait.h>

#define MAXLINE     4096

static int nchildren;
static pid_t *pids;
static struct flock	lock_it, unlock_it;
static int		lock_fd = -1;
		/* fcntl() will fail if my_lock_init() not called */

// implementa il controllo di segnali per gestire poi i figli
typedef void Sigfunc(int);
Sigfunc* signal(int signum, Sigfunc *handler);

// implementa la logica di lock/unlock
void my_lock_init(char *pathname)
{
  char	lock_file[1024];

  /* must copy caller's string, in case it's a constant */
  strncpy(lock_file, pathname, sizeof(lock_file));
  if ( (lock_fd = mkstemp(lock_file)) < 0) {
    fprintf(stderr, "errore in mkstemp");
    exit(1);
  }

  if (unlink(lock_file) == -1) { 	/* but lock_fd remains open */
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

void my_lock_wait()
{
  int rc;

  while ( (rc = fcntl(lock_fd, F_SETLKW, &lock_it)) < 0) {
    if (errno == EINTR)
      continue;
    else {
      fprintf(stderr, "errore fcntl in my_lock_wait");
      exit(1);
    }
  }
}

void my_lock_release()
{
    if (fcntl(lock_fd, F_SETLKW, &unlock_it) < 0) {
      fprintf(stderr, "errore fcntl in my_lock_release");
      exit(1);
    }
}

void web_child(int sockfd)
{
  char buff[MAXLINE];
  //ascolto il msg del client
  /*
  while (1) {
	  len = sizeof(addr);
	  if ( (recvfrom(sockfd, buff, MAXLINE, 0, (struct sockaddr *)&addr, &len)) < 0) {
		perror("errore in recvfrom");
		exit(-1);
	  }
// 	qui devo implementare la logica di risposta del server 

// server per rispondere alla richiesta del client
	if (sendto(sockfd, buff, strlen(buff), 0, (struct sockaddr *)&addr, sizeof(addr)) < 0){
		perror("errore in sendto");
		exit(-1);
	}
}
*/
  printf("sono il child %d \n",getpid());
}

void child_main(int i, int listenfd, int addrlen){
	int connfd;
	socklen_t clilen;
	struct sockaddr *cliaddr;
	if ( (cliaddr = (struct sockaddr *)malloc(addrlen)) == NULL) {
   		fprintf(stderr, "errore in malloc");
    		exit(1);
	}
	printf("child %ld starting\n", (long) getpid());
    	for ( ; ; ) {
		clilen = addrlen;
		my_lock_wait(); /* my_lock_wait() usa fcntl() */
		if ( (connfd = accept(listenfd, cliaddr, &clilen)) == -1) {
      			perror("errore in accept \n ");
      			exit(1);
		}

		my_lock_release();
		web_child(connfd); /* processa la richiesta */
		if (close(connfd) == -1) {
			perror("errore in close");
     			exit(1);
		}
	}
}

// creo i processi figli 
pid_t child_make(int i, int listenfd, int addrlen){
	pid_t pid;
	if((pid=fork()>0))
		return(pid);// processo padre
	child_main(i,listenfd,addrlen);// non ritorna mai
}
// blocco i segnali esterni tranne alcuni che decido ctrl
Sigfunc *signal(int signum, Sigfunc *func)
{
  struct sigaction      act, oact;
        /* la struttura sigaction memorizza informazioni riguardanti la
        manipolazione del segnale */

  act.sa_handler = func;
  sigemptyset(&act.sa_mask);    /* non occorre bloccare nessun altro segnale */
  act.sa_flags = 0;
  if (signum != SIGALRM)
     act.sa_flags |= SA_RESTART;
  if (sigaction(signum, &act, &oact) < 0)
    return(SIG_ERR);
  return(oact.sa_handler);
}
// termina i figli non ho stato di zombie 
void sig_int(int signo)
{
  int	i;
	/* terminate all children */
  for (i = 0; i < nchildren; i++)
    kill(pids[i], SIGTERM);
  while (wait(NULL) > 0) ;	/* wait for all children */
		
  if (errno != ECHILD)  {
    fprintf(stderr, "errore in wait");
    exit(1);
  }
  exit(0);
}

int main(int argc, char **argv) {
	if(argc != 4 ){
		fprintf(stderr," utilizzo:<variabile P, TO , SERV_Port > \n");
		exit(1);
	}
// variabili richieste dalla traccia
int p=atoi(argv[1]);
int TO=atoi(argv[2]);
int SERV_PORT=atoi(argv[3]);

	if(SERV_PORT<255){
		fprintf(stderr,"inserire num. porta >255");
		exit(1);
	}
// variabili per il prefork	
int listenfd, i;
socklen_t addrlen;
//queste due nemmeno servono
void sig_int(int);
pid_t child_make(int, int, int);


// variabili normali
  int nchildren=10; // numeri di figli qui possiamo rendere le cose dinamiche
  int sockfd;
  socklen_t len=sizeof(struct sockaddr_in);
  struct sockaddr_in addr;

  if ((listenfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) { /* crea il socket */
    perror("errore in socket");
    exit(1);
  }

  memset((void *)&addr, 0, sizeof(addr));
  addr.sin_family = AF_INET;
  addr.sin_addr.s_addr = htonl(INADDR_ANY); /* il server accetta pacchetti su una qualunque delle sue interfacce di rete */
  addr.sin_port = htons(SERV_PORT); /* numero di porta del server */

  /* assegna l'indirizzo al socket */
  if (bind(listenfd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
    perror("errore in bind");
    exit(1);
  }
// implemento la logica della prefork con file locking

// qui alloco un area di memoria per mantenere i pid dei child
pids = (pid_t *)calloc(nchildren, sizeof(pid_t));
  if (pids == NULL) 
  { 
    fprintf(stderr, "errore in calloc");
    exit(1);
  }

  my_lock_init(("/tmp/lock.XXXXXX")); /* one lock file for all children */
  for (i = 0; i < nchildren; i++){
	  //inserisco nell array pids i pid dei figli che mi ritornano dalla child_make
    pids[i] = child_make(i, listenfd, addrlen);	/* parent returns */
  }
  if (signal(SIGINT, sig_int) == SIG_ERR) {
    fprintf(stderr, "errore in signal");
    exit(1);
  }

  for ( ; ; )
    pause();	/* everything done by children */
}













