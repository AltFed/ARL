#include <sys/types.h> 
#include <sys/socket.h> 
#include <arpa/inet.h>
#include <fcntl.h>
#include <unistd.h> 
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <signal.h>


#define SERV_PORT   5193 
#define MAXLINE	1024
int len=10;	// len per dimensione buffer send
int sockfd;	// descrittore alla socket creata per comunicare con il server 
struct    sockaddr_in   servaddr;

// gestisce il segnale di alarm (TIMEOUT)
void sig_handler(int signum){
	// qui devo ridurre il len di quanto mi pare 
}

// implementa il controllo della congestione
void congest(){

}

// funzione che implementare la send to server
void csend(char *buff,int i){
	if(i==1){
		// apro il file che voglio mandare lo leggo e poi invio al server
	}
// Invia al server il pacchetto di richiesta
  if (sendto(sockfd, buff, strlen(buff), 0, (struct sockaddr *) &servaddr, sizeof(servaddr)) < 0) {
    perror("errore in sendto");
    exit(1);
  }
// per implementare il timeout uso SIGALRM

alarm(2);  // Scheduled alarm after 2 seconds
		 

// Legge dal socket il pacchetto di risposta 
 if(recvfrom(sockfd, buff, len , 0, (struct sockaddr *) &servaddr, sizeof(servaddr) < 0)){
    perror("errore in recvfrom");
    exit(1);
  }

// invoco la funzione congest per gestire l'arrivo di un ack aumento len 
  congest();

}

// concateno la stringa e creo il comando get da inviare al server
void cget(){

	char * buff=malloc(MAXLINE);
	char b[MAXLINE-6];
	printf("inserire nome file \n");

	fscanf(stdin,"%s",b);
	snprintf(buff,MAXLINE,"get %s",b);
	csend(buff,0);

 }

// creo il comando list 
void clist(){
	char * buff=malloc(MAXLINE);
	buff="list";
	csend(buff,0);

}

// creo il comando put 
void cput(){
	char * buff=malloc(MAXLINE);
	char b[MAXLINE-6];
	printf("inserire nome file \n");

	fscanf(stdin,"%s",b);

	snprintf(buff,MAXLINE,"put %s",b);

	// metto un numero perchè cosi gestisco i casi in cui richiedo solo dal caso in cui devo inviare il file e aprire quindi un file leggerlo ecc 
	csend(buff,1);

}

// gestisco la richiesta dell'utente 
void req(){
	int a;
   	while(1){
	printf("Inserire numero: -1 exit \n");
	fscanf(stdin,"%d",&a);
	switch (a){
		case 0:
			cget();
			break;

		case 1:
			clist();
			break;	

		case 2:
			cput();
			break;
		case -1:
			return;
	}
	}
}

int main(int argc, char *argv[ ]) {
  char  recvline[MAXLINE + 1];
  if (argc != 2) {  //controlla numero degli argomenti 
    fprintf(stderr, "utilizzo: <indirizzo IP server>\n");
    exit(1);
  }
  
// implemento il controllore del segnale 
  signal(SIGALRM,sig_handler); // Register signal handler
			       //
  if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {  //crea il socket 
    perror("errore in socket");
    exit(1);
  }

  memset((void *)&servaddr, 0, sizeof(servaddr));      // azzera servaddr 
						       //
  servaddr.sin_family = AF_INET;       // assegna il tipo di indirizzo 
				       //
  servaddr.sin_port = htons(SERV_PORT);  // assegna la porta del server 
					 //
  // assegna l'indirizzo del server prendendolo dalla riga di comando. L'indirizzo è una stringa da convertire in intero secondo network byte order. 
  if (inet_pton(AF_INET, argv[1], &servaddr.sin_addr) <= 0) {
                // inet_pton (p=presentation) vale anche per indirizzi IPv6 
    fprintf(stderr, "errore in inet_pton per %s", argv[1]);
    exit(1);
  }

  //invoco la funzione per gestire le richieste dell'utente
  req();

 }

