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
// gestico la list 
void recvfile(){
	printf("recvfile alive\n");
	char *rcv_buff=malloc(MAXLINE);
	int n=1,i=0,k=1;
	char *seq_ack=malloc(MAXLINE);
	sprintf(seq_ack,"%d",k);
	while(stay){
		rcv_buff[0]='\0';
		if (( n=(recvfrom(sockfd, rcv_buff, MAXLINE, 0, (struct sockaddr *)&servaddr, &addrlen )))< 0) {
        		perror("errore in recvfrom");
       			exit(1);
		}
        if(n!=0){
		printf("valore di n->%d \n",n);
		rcv_buff[strlen(rcv_buff)+1]='\0';
		while(rcv_buff[i] != ' '){
			i++;
		}
		if(strncmp(seq_ack,rcv_buff,i+1)){
			printf("ack che mi aspetto-> %s ||||| ack ricevuto ->%s \n",seq_ack,rcv_buff);
        
		}else{
			perror("errore ack seq num");
			exit(1);
		}
		printf("directory -> %s\n",rcv_buff+i+1);
        
        }else if(n==0){
            stay=false;
        }
    }
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

  char *snd_buff = malloc(MAXLINE);
  char *rcv_buff = malloc(MAXLINE);
  char *ack = malloc(10);
  char *seq = malloc(10);
  int k=0,i=0,n=0;
      printf("comando da inviare ---> %s\n",pkt);

      sprintf(seq, "%d\n", k);
      seq[sizeof(k)+1] = '\0';
      printf("ecco il seq---> %s\n",seq); 

      strcat(snd_buff, seq);
      strcat(snd_buff, pkt);
      strcpy(pkt_send,pkt);

      sprintf(snd_buff, "%s%s",seq, pkt);
      snd_buff[strlen(snd_buff) + 1] = '\0';

      printf("ecco il buff che passo al server \n\n%s\n\n", snd_buff);

      // Invia al server il pacchetto di richiesta
      if (sendto(sockfd, snd_buff, strlen(snd_buff), 0,(struct sockaddr *)&servaddr, addrlen) < 0) {
        perror("errore in sendto");
        exit(1);
      }
      free(snd_buff);

      // Legge dal socket il pacchetto di risposta
	
      if (recvfrom(sockfd, rcv_buff, MAXLINE, 0, (struct sockaddr *)&servaddr, &addrlen )<0) {   
        perror("errore in recvfrom");
        exit(1);
      }

      printf("rcv_buff -->> \n%s\n",rcv_buff);

   // qui copio l'ack inviato a numero dal server 
      while(rcv_buff[i] != '\n'){
          i++;
      }
      strncpy(ack,rcv_buff, i);
      ack[i]='\0';

      printf("ack rivecuto %s -> ack che mi aspettavo %s \n",ack,seq);
      fflush(stdout);
      
      int j = 0;
      while(rcv_buff[i+j+1] != '\n'){
          j++;
      }
      if(!strncmp(ack,seq,i) && !strncmp(rcv_buff+i+1, "-1", j)){
        
          printf("Errore : %s\n",rcv_buff+i+j);
      }
      else if(!strncmp(ack,seq,i) ){
          puts("OK!");
          printf("Phrase : %s\n",rcv_buff+i+j);
              }
      

      free(ack);
      free(seq);
      free(rcv_buff);

    
      
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
 //file_receive();  
}

// creo il comando list
void clist() {
  char *buff = malloc(MAXLINE);
  buff="list";
  command_send(buff);
 //file_receive();
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
  int a;
  while (1) {
    printf("Inserire numero:\nget=0\nlist=1\nput=2\nexit=-1\n");
    fscanf(stdin, "%d", &a);
    switch (a) {
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
