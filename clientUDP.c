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
#define GET 0
#define PUT 1
#define LIST 2

int len=100;	// len per dimensione buffer send
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
void csend(char *pkt,int mode){
char * buff=malloc(MAXLINE);
char seq[2];
int k=8;
sprintf(seq,"%d",k);
/*
    switch(mode){
        case GET :
                // invio nome del file dal prendere, invoco funzione per ricevere ( congest ?? )
                
            break;
        case PUT:
                // apro file, bufferizzo e invio, mando in volo max N pacchetti, finche non ho ack. Come faccio a leggere se sono in write? 
                
            break;
        case LIST:
                // invio semplicemente comando list a server, il server crea un file con un elenco, mi faccio spedire il file e lo printo riga per riga.
                
            break;
    }

*/
strcat(buff,seq);
strcat(buff,pkt);
buff[strlen(buff)+1]='\0';
printf("ecco il buff che passo al server %s\n",buff);
// Invia al server il pacchetto di richiesta
  if (sendto(sockfd, buff, len, 0, (struct sockaddr *) &servaddr, sizeof(servaddr)) < 0) {
    perror("errore in sendto");
    exit(1);
  }
// per implementare il timeout uso SIGALRM

//alarm(0.5);  // Scheduled alarm after 500ms
		 

// Legge dal socket il pacchetto di risposta 
 if(recvfrom(sockfd, pkt, len , 0, (struct sockaddr *) &servaddr, sizeof(servaddr) < 0)){
    perror("errore in recvfrom");
    exit(1);
  }
	printf("ho ricevuto dal server %s\n",pkt);

// invoco la funzione congest per gestire l'arrivo di un ack aumento len 
  //congest();
}





// concateno la stringa e creo il comando get da inviare al server
void cget(){

	char * buff=malloc(MAXLINE);
	char b[MAXLINE-6];
	printf("inserire nome file \n");

	fscanf(stdin,"%s",b);
	snprintf(buff,MAXLINE,"get %s",b);
	csend(buff,GET);

 }

// creo il comando list 
void clist(){
	char * buff=malloc(MAXLINE);
	buff="list";
	csend(buff,LIST);

}

// creo il comando put 
void cput(){
	char * buff=malloc(MAXLINE);
	char b[MAXLINE-6];
	printf("inserire nome file \n");

	fscanf(stdin,"%s",b);

	snprintf(buff,MAXLINE,"put %s",b);

	// metto un numero perchè cosi gestisco i casi in cui richiedo solo dal caso in cui devo inviare il file e aprire quindi un file leggerlo ecc 
	csend(buff,PUT);

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

