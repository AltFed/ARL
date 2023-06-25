#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/time.h>
#include <stdbool.h>
#define DIM_MAX 5
#define SERV_PORT 5193
#define MAXLINE 1024
#define TIMEOUT_MS      100

int dim_send = 5;

char pkt_send[MAXLINE]; 

int sockfd;  // descrittore alla socket creata per comunicare con il server

struct sockaddr_in servaddr;

bool stay = true;

void command_send(char *);

socklen_t addrlen = sizeof(struct sockaddr_in);

// implementa il controllo della congestione
void congest() {
  if (dim_send > 2) {
    dim_send--;
  }
}

// gestisce il segnale di alarm (TIMEOUT)
void sig_handler(int signum) {
  // qui devo ridurre il len di quanto mi pare
  printf("sig_hanlder ======\n");
  command_send(pkt_send);
}

//implemento la rcv del comando get 
void rcv_get(char *file){

}

//implemento la snd del comando put 
void snd_put(char *file){

}

//implemento la rcv del comando list 
void rcv_list(){
	printf("rcv_list alive\n");
	char rcv_buff[MAXLINE];
	char snd_buff[30];
	char number_pkt[30];
	int n=1,i=0,k=1;
	while(stay){
		snd_buff[0]='\0';
		number_pkt[0]='\0';
		rcv_buff[0]='\0';
		if ((recvfrom(sockfd, rcv_buff, MAXLINE, 0, (struct sockaddr *)&servaddr, &addrlen ))< 0) {
        		perror("errore in recvfrom");
       			exit(1);
		}
		while(rcv_buff[i] != '\n'){
			i++;
		}
		strncpy(number_pkt,rcv_buff,i);	
		number_pkt[i]='\0';
	// END == terminatore pkt inviati 
	printf("NUM RICEVUTO-> %s\n",number_pkt);

	printf("NUM CHE VOGLIO->%d\n",n);

        if(!strcmp(number_pkt,"END")){

		printf("Terminatore ricevuto\n");

		printf("%s\n",rcv_buff+i);
		fflush(stdout);

		stay=false;

		//confronto il numero ricevuto e quello che mi aspetto

        }else if(n==strtol(number_pkt,NULL,10) && stay == true ){

		printf("valore di n->%d \n",n);

		printf("%s\n",rcv_buff+i);

		fflush(stdout);
		//incremento n
		n++;
        }
	else if( n != strtol(number_pkt,NULL,10) && stay == true ){
		//gestire ack non in ordine ES: inviamo un ack al sender e gli diciamo di inviare tutto dopo quel numero 
		printf("Numero ricevuto diverso da quello che mi aspettavo\n");
		sprintf(snd_buff,"%d\n",n);
		fflush(stdout);
		if (sendto(sockfd, snd_buff, strlen(snd_buff), 0,(struct sockaddr *)&servaddr, addrlen) < 0) {
        		perror("errore in sendto");
        			exit(1);
		}
	}
    }
	sleep(1);
}
void file_send(char *file_name){
	/*
 * bool stay = true;
   while (stay) {
    if (i < dim_send) {
      i++;
    if (recvfrom(sockfd, rcv_buff, MAXLINE, MSG_DONTWAIT,
                   (struct sockaddr *)&servaddr, sizeof(servaddr) < 0)) {
        perror("errore in recvfrom");
        exit(1);
      }
      while (rcv_buff[i] != ' ') {
        i++;
      }

      strncpy(rcv_seq, rcv_buff, i);

      if (strcmp(seq, rcv_seq)) {
        alarm(0);
      }
      printf("ho ricevuto dal server %s\n", pkt);

      // invoco la funzione congest per gestire l'arrivo di un ack aumento len
      // congest();
      // }
      // }
      // */

}
// funzione che implementare la send to server
void command_send(char *pkt){ 
  char command_buff[100];
  char nome_file[256];
  char snd_buff[MAXLINE];
  char rcv_buff[MAXLINE];
  char ack [10];
  char seq [10];
  int k=0,i=0,n=0,r=0;

     while( pkt[r] != ' '){
	     r++;
     }
      // copio il comando lo uso dopo per avviare la funzione corretta
      strncpy(command_buff,pkt,r);

      printf("comando da inviare ---> %s\n",pkt);

      sprintf(seq, "%d\n", k);

      seq[strlen(seq)+1] = '\0';

      printf("ecco il seq---> %s\n",seq); 

      strcat(snd_buff, seq);

      strcat(snd_buff, pkt);

      strcpy(pkt_send,pkt);

      sprintf(snd_buff, "%s%s",seq, pkt);
      //controllo in caso il snd_buff è pieno metto il terminatore alla fine 
      if(strlen(snd_buff)<MAXLINE){

      snd_buff[strlen(snd_buff) + 1] = '\0';

      }else{
	      snd_buff[strlen(snd_buff)] = '\0';
      }

      printf("ecco il buff che passo al server \n\n%s\n\n", snd_buff);

      fflush(stdout);

      // Invia al server il pacchetto di richiesta
      if (sendto(sockfd, snd_buff, strlen(snd_buff), 0,(struct sockaddr *)&servaddr, addrlen) < 0) {
        perror("errore in sendto");
        exit(1);
      }

      // Legge dal socket il pacchetto di risposta
	
      if (recvfrom(sockfd, rcv_buff, MAXLINE, 0, (struct sockaddr *)&servaddr, &addrlen )<0) {   
        perror("errore in recvfrom");
        exit(1);
      }

      
      while(rcv_buff[i] != '\n'){
          i++;
      }
      strncpy(ack,rcv_buff, i);

      ack[i]='\0';

      printf("Command_send: ack rivecuto %s -> ack che mi aspettavo %s \n",ack,seq);

      fflush(stdout);
     if(!strcmp(ack,seq)){
	     printf("\nnumero ricevuto diverso ritorno su \n");
	     sleep(10);
	     command_send(pkt);
     }
      int j = 0;

      while(rcv_buff[i+j+1] != '\n'){
          j++;
      }

      if(!strncmp(ack,seq,i) && !strncmp(rcv_buff+i+1, "-1", j)){
        
          printf("Errore : %s\n",rcv_buff+i+j);
	  //gestire errore 
      }
      else if(!strncmp(ack,seq,i) ){

          puts("OK!");
          printf("Code + Phrase : %s\n",rcv_buff+i+j);

	  //implento la list 
	  if(!strcmp(command_buff,"list")){
		 rcv_list();
		 printf("rcv_list return\n");
	  }

	  //implento la get
	  else if (!strcmp(command_buff,"get")){

		  // copio il nome del file 
		  strcpy(nome_file,pkt+r);
		  rcv_get(nome_file);
	  }

	  //implemento la put 
	  else if (!strcmp(command_buff,"put")){
		  //copio il nome del file 
		  strcpy(nome_file,pkt+r);
		  snd_put(nome_file);
	  }
              }

          printf("sto uscendo dalla command\n"); 
}

// concateno la stringa e creo il comando get da inviare al server
void cget() {
  char *buff = malloc(MAXLINE);
  char b[MAXLINE - 6];
  printf("inserire nome file \n");

  fscanf(stdin, "%s", b);
  snprintf(buff, MAXLINE, "get %s", b);
  command_send(buff);
  free(buff);
  
}
// creo il comando put
void cput() {
  char *buff = malloc(MAXLINE);
  char b[MAXLINE - 6];
  printf("inserire nome file \n");

  fscanf(stdin, "%s", b);

  snprintf(buff, MAXLINE, "put %s", b);

  // metto un numero perchè cosi gestisco i casi in cui richiedo solo dal caso
  // in cui devo inviare il file e aprire quindi un file leggerlo ecc
  command_send(buff);
  //file_send(b);
  free(buff);	
}

// gestisco la richiesta dell'utente
void req() {
  int a=0;
  while (1) {
    printf("\nInserire numero:\nget=0\nlist=1\nput=2\nexit=-1\n");
    fscanf(stdin, "%d", &a);
    switch (a) {
      case 0:
        cget();
        break;

      case 1:
        command_send("list");	//passo direttamento la list alla command_send senza usare una funzione ausiliaria
        fflush(stdout);
	break;

      case 2:
        cput();
        break;
      case -1:
        return;
    }
  }
}

int main(int argc, char *argv[]) {
  char recvline[MAXLINE + 1];
  if (argc != 2) {  // controlla numero degli argomenti
    fprintf(stderr, "utilizzo: <indirizzo IP server>\n");
    exit(1);
  }

  // implemento il controllore del segnale
  signal(SIGALRM, sig_handler);  // Register signal handler
                                 //
  if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {  // crea il socket
    perror("errore in socket");
    exit(1);
  }

  memset((void *)&servaddr, 0, sizeof(servaddr));  // azzera servaddr
                                                   //
  servaddr.sin_family = AF_INET;         // assegna il tipo di indirizzo
                                         //
  servaddr.sin_port = htons(SERV_PORT);  // assegna la porta del server
                                         //
  // assegna l'indirizzo del server prendendolo dalla riga di comando.
  // L'indirizzo è una stringa da convertire in intero secondo network byte
  // order.
  if (inet_pton(AF_INET, argv[1], &servaddr.sin_addr) <= 0) {
    // inet_pton (p=presentation) vale anche per indirizzi IPv6
    fprintf(stderr, "errore in inet_pton per %s", argv[1]);
    exit(1);
  }

  // invoco la funzione per gestire le richieste dell'utente
  req();
}
