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

#define DIM_MAX 5 
#define SERV_PORT   5193 
#define MAXLINE	1024

int dim_send=5;

int sockfd;	// descrittore alla socket creata per comunicare con il server 
struct    sockaddr_in   servaddr;

// gestisce il segnale di alarm (TIMEOUT)
void sig_handler(int signum){
// qui devo ridurre il len di quanto mi pare
	congest();
}

// implementa il controllo della congestione
void congest(){
	if(dim_send>2){
		dim_send--;
	}	
}

// funzione che implementare la send to server
void csend(char *pkt){
bool stay=true;
char * snd_buff=malloc(MAXLINE);
char * rcv_buff=malloc(MAXLINE);
char * ack=malloc(100);
char seq[100];
int k=0,i=0;
while(stay){
	if(i<dim_send){
		i++;
		sprintf(seq,"%d ",k);
		k++;
		strcat(snd_buff,seq);

		strcat(snd_buff,pkt);

		buff[strlen(buff)+1]='\0';
		//temp=read_file();
		//strcat(buff+strlen(buff),temp);
		printf("ecco il buff che passo al server %s\n",buff);

// Invia al server il pacchetto di richiesta

  		if (sendto(sockfd, snd_buff, sizeof(snd_buff) , MSG_DONTWAIT, (struct sockaddr *) &servaddr, sizeof(servaddr)) < 0) {
    			perror("errore in sendto");
    			exit(1);
 			 }

	// per implementare il timeout uso SIGALRM

		//alarm(0.5);  // Scheduled alarm after 500ms		
// Legge dal socket il pacchetto di risposta 

 if(recvfrom(sockfd, rcv_buff, MAXLINE, MSG_DONTWAIT, (struct sockaddr *) &servaddr, sizeof(servaddr) < 0)){
    perror("errore in recvfrom");
    exit(1);
  }
 while(rcv_buff[i] != ' '){
	 i++;
 }

 strncpy(rcv_seq,rcv_buff,i);

 if(strcmp(seq,rcv_seq)){
	 alarm(0);
 }
 printf("ho ricevuto dal server %s\n",pkt);

// invoco la funzione congest per gestire l'arrivo di un ack aumento len 
  //congest();
	}
}
}





// concateno la stringa e creo il comando get da inviare al server
void cget(){

	char * buff=malloc(MAXLINE);
	char b[MAXLINE-6];
	printf("inserire nome file \n");

	fscanf(stdin,"%s",b);
	snprintf(buff,MAXLINE,"get %s",b);
	csend(buff);

 }

// creo il comando list 
void clist(){
	char * buff=malloc(MAXLINE);
	buff="list";
	csend(buff);

}

// creo il comando put 
void cput(){
	char * buff=malloc(MAXLINE);
	char b[MAXLINE-6];
	printf("inserire nome file \n");

	fscanf(stdin,"%s",b);

	snprintf(buff,MAXLINE,"put %s",b);

	// metto un numero perchè cosi gestisco i casi in cui richiedo solo dal caso in cui devo inviare il file e aprire quindi un file leggerlo ecc 
	csend(buff);

}

// gestisco la richiesta dell'utente 
void req(){
	int a;
   	while(1){
	printf("Inserire numero 0=get 1=list 2=put : -1 exit \n");
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

