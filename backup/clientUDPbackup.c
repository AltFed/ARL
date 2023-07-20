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
#define MAXLINE 4096
#define TIMEOUT_MS      100

int dim_send = 5;

char pkt_send[MAXLINE]; 

int sockfd;  // descrittore alla socket creata per comunicare con il server

struct sockaddr_in servaddr;

void command_send(char *);

socklen_t addrlen = sizeof(struct sockaddr_in);
struct st_pkt
{
	int ack;
	char pl[MAXLINE];
};
// serve 
void cget();

// gestisce il segnale di alarm (TIMEOUT)
void sig_handler(int signum) {
  // qui devo ridurre il len di quanto mi pare
  printf("sig_hanlder ======\n");
  command_send(pkt_send);
}

//implemento la rcv del comando get 
void rcv_get(char *file){
	struct st_pkt pkt;
	printf("rcv_get alive\n");
	printf("ecco il nome del file %s\n",file);
	FILE * fptr;

	//creo il file se già esiste lo cancello tanto voglio quello aggiornato 
	int n=1,i=0;

	pkt.ack=n;

	bool stay=true;
	if((fptr = fopen("ciao","r+")) == NULL){
		perror("Error opening file");
		exit(1);
		}
	while(stay){	
		if ((recvfrom(sockfd,&pkt, sizeof(pkt), 0, (struct sockaddr *)&servaddr,&addrlen ))< 0) {
        		perror("errore in recvfrom");
       			exit(1);
		}		
	// END == terminatore pkt inviati 
	
	printf("NUM RICEVUTO-> %d\n",pkt.ack);

	printf("NUM CHE VOGLIO->%d\n",n);

	printf("\n PKT Payload %s\n",pkt.pl);
	fflush(stdout);
	//se ricevo END e con numero corretto invio la chiusura e stop 
        if(!strcmp(pkt.pl,"END") && pkt.ack == n){

		printf("\nTerminatore ricevuto\n");
		fflush(stdout);

		//devo aprire il file e scriverci l'ultimo pkt 	
		stay=false;

		//invio  ACK di fine ricezione al sender
		sprintf(pkt.pl,"OK");
		//invio il numero di seq del pkt risolvo in caso mi inviasse un unico pkt con numero sbagliato
		printf("\nPKT---> %d %s\n",pkt.ack,pkt.pl);
		if (sendto(sockfd,&pkt, sizeof(pkt), 0,(struct sockaddr *)&servaddr,addrlen) < 0) {
        			perror("errore in sendto");
        			exit(1);
		}
		//confronto il numero ricevuto e quello che mi aspetto
        }else if(n == pkt.ack && stay == true && strcmp(pkt.pl,"END")){
		//ogni 2 pkt invio un ack cum
		if(i == 1){
			printf("\nInvio ack cum\n");
			fflush(stdout);
			i= i % 1;
			pkt.ack=n;
			//forse qui posso mandare solo il pkt.ack 
			if (sendto(sockfd,&pkt, sizeof(pkt), 0,(struct sockaddr *)&servaddr,addrlen) < 0) {
        			perror("errore in sendto");
        			exit(1);
			}
		}
		printf("\nScrivo il msg sul file ->%s\n",pkt.pl);

		fflush(stdout);

		if((fwrite(pkt.pl,strlen(pkt.pl),1,fptr) <0 )){
				perror("Error in write rcv_get\n");
				exit(1);
				}
		//incremento i mii implementa ack cum
		i++;
		//incremento n
		n++;
	
        }

	// se arriva un pkt fuori ordine invio subito ack non faccio la bufferizzazione lato rcv 
	else if( n != pkt.ack && stay == true /*&& strcmp(pkt.pl,"END" )*/){

		//gestire ack non in ordine ES: inviamo un ack al sender e gli diciamo di inviare tutto dopo quel numero 
		printf("Numero ricevuto diverso da quello che mi aspettavo\n");
		// invio al sender un ack comulativo fino a dove ho ricevuto
		pkt.ack=n;
		if (sendto(sockfd,&pkt, sizeof(pkt), 0,(struct sockaddr *)&servaddr,addrlen) < 0) {
        		perror("errore in sendto");
        		exit(1);
		}
		// END e ack diverso 
	}/*else if(!strcmp(pkt.pl,"END") && pkt.ack != n){
		printf("=============");
		sprintf(pkt.pl,"OK");

		pkt.ack=n;
		printf("Ricevuto END con ack diverso\n");
		if (sendto(sockfd,&pkt, sizeof(pkt), 0,(struct sockaddr *)&servaddr,addrlen) < 0) {
        		perror("errore in sendto");
        			exit(1);
		}
	}
	*/
	}
	printf("\nreturn rcv_get\n");
	fflush(stdout);
}

//implemento la snd del comando put 
void snd_put(char *str){
  FILE *file;
  char snd_buff[MAXLINE+2];
  char temp[MAXLINE];
  char rcv_buff[100];
  char path_file[200];
  char *fin="END\n";
  bool stay=true
  bool error=true;
  int t=1;    
  sprintf(path_file,"Server_Files/%s",str);
  printf("path %s\n",path_file);
  if((file = fopen("ciao", "r+")) == NULL){
    printf("Errore in open del file\n");
    exit(1);
  }
  while(stay){
       while(fgets(temp, MAXLINE,file)) {
	 // inserisco il numero al pkt che invio 
	 fflush(stdout);
	 sprintf(snd_buff,"%d\n",t);
  	 sprintf(snd_buff+2,"%s",temp);
	snd_buff[strlen(snd_buff)]='\0';
	  printf("PKT SIZE %ld\n",strlen(snd_buff));
	  printf("invio il pkt %s",snd_buff);
	  fflush(stdout);
 	  if ((sendto(sockfd, snd_buff,strlen(snd_buff), 0, (struct sockaddr *)&servaddr,addrlen)) < 0)  {
                			perror("errore in sendto");
                			exit(1);
		 }
		 t++;
	  }
  if(feof(file)) {
	stay=false;
	 printf("Chiusura comando put \n");

	 printf("\nULTIMO PKT\n");
	
	 fflush(stdout);

	// +2: perchè tolgo il numero 
        // -2: perchè copio i bit ma devo togliere il numero  che vengono contati dallo strlen
	printf("\nInvio al rcv il pkt\n %s\n",fin);

	fflush(stdout);
	if ((sendto(sockfd,fin ,strlen(fin), 0, (struct sockaddr *)&servaddr,addrlen)) < 0)  {
               perror("errore in sendto");
               exit(1);
	}
	if (recvfrom(sockfd, rcv_buff, MAXLINE, 0, (struct sockaddr *)&servaddr,&addrlen ) <0 ) {   
       		 perror("errore in recvfrom");
        	 exit(1);
		}
	// implemento gestione fine connessione mi aspetto un pkt con solo OK dal receiver
	printf("pkt ricevuto %s\n",rcv_buff);
	if(!strcmp(rcv_buff,"OK\n")){
		printf("\nConnessione chiusa correttamente ricevuto ACK %s\n",rcv_buff);
	}else{
		//gestione errore ritrasmettere pkt 

  }
  }
  }
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
		
  printf("\nSnd_put return\n");
}

//implemento la rcv del comando list 
void rcv_list(){
	printf("rcv_list alive\n");
	char rcv_buff[MAXLINE];
	char snd_buff[30];
	char number_pkt[30];
	int n=1,i=0,k=1;
	bool stay=true;
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
		//invio  ACK di fine ricezione al sender 
		sprintf(snd_buff,"OK\n");

		if (sendto(sockfd, snd_buff, strlen(snd_buff), 0,(struct sockaddr *)&servaddr,addrlen) < 0) {
        			perror("errore in sendto");
        			exit(1);
		}
		//fare sul server questa cosa |||||||||||||||
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
        	if (sendto(sockfd, snd_buff, strlen(snd_buff), 0,(struct sockaddr *)&servaddr,addrlen) < 0) {
        		perror("errore in sendto");
        			exit(1);
		}
	}
    }
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
      if (sendto(sockfd, snd_buff, strlen(snd_buff), 0,(struct sockaddr *)&servaddr,addrlen) < 0) {
        perror("errore in sendto");
        exit(1);
      }

      // Legge dal socket il pacchetto di risposta
	
      if (recvfrom(sockfd, rcv_buff, MAXLINE, 0, (struct sockaddr *)&servaddr,&addrlen )<0) {   
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
	     command_send(pkt);
     }
      int j = 0;

      while(rcv_buff[i+j+1] != '\n'){
          j++;
      }

      if(!strncmp(ack,seq,i) && !strncmp(rcv_buff+i+1, "-1", j)){
        
          printf("Errore : %s\n",rcv_buff+i+j);
	  printf("\nFINE\n");
	  //ritorno nella cget per ottenere il nuovo nome file 
	  //forse può andare bene creo un loop si ma dovrebbe uscire bene 
	  cget();
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
		  strcpy(nome_file,pkt+r+1);
		  rcv_get(nome_file);
		  printf("rcv_get return\n");
	  }

	  //implemento la put 
	  else if (!strcmp(command_buff,"put")){
		  //copio il nome del file 
		  strcpy(nome_file,pkt+r+1);
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
	// chiusura connessione
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
