#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>

#define SERV_PORT 5193
#define MAXLINE 4096

char pkt_send[MAXLINE];
bool loop = false;
int dim = 0;
int free_dim = 0;
int sockfd; // descrittore alla socket creata per comunicare con il server
struct sockaddr_in servaddr;
void command_send(char *, char *);

socklen_t addrlen = sizeof(struct sockaddr_in);
// struct pkt
struct st_pkt {

  int ack;
  int finbit;
  char pl[MAXLINE];
  int rwnd;
};
struct st_pkt *rcv_win;
void cget();
void req();

// implemento la rcv del comando get
void rcv_get(char *file) {
  struct st_pkt pkt;
  if ((rcv_win = malloc(sizeof(struct st_pkt) * dim)) == NULL) {
    perror("Error malloc");
    exit(1);
  }
  printf("\n Client : Get function alive\n");
  fflush(stdout);
  FILE *fptr;
  // creo il file se già esiste lo cancello tanto voglio quello aggiornato
  int n = 0, i = 0, maxseqnum = 0;
  bool stay = true, different = false;

  if ((fptr = fopen(file, "w+")) == NULL) {
    perror("Error opening file");
    exit(1);
  }
  while (stay) {
    // incremento n
    if (!different) {
      n++;
    }
    if ((recvfrom(sockfd, &pkt, sizeof(pkt), 0, (struct sockaddr *)&servaddr,
                  &addrlen)) < 0) {
      perror("errore in recvfrom");
      exit(1);
    }
    // se mi arriva un ack che ho già salvato non lo mantengo nell'array
    if (maxseqnum < pkt.ack) {
      maxseqnum = pkt.ack;
    }
    // finbit == 1 allora chiudo la connessione

    if (pkt.finbit == 1 && pkt.ack == n) {
      rcv_win[i] = pkt;
      i++;
      // cicliclo
      i = i % dim;
       free_dim--;
      printf("\n Client : Server close connection \n");
      // invio subito ack cum
      pkt.ack = n;
      pkt.finbit = 1;
      pkt.rwnd = free_dim;
      printf("\n Client : Confermo chiusura\n");
      //printf("\nACK --> %d   PL %s  \nN %d\n", pkt.ack, pkt.pl, n);
      fflush(stdout);

      if (sendto(sockfd, &pkt, sizeof(pkt), 0, (struct sockaddr *)&servaddr,
                 addrlen) < 0) {
        perror("errore in sendto");
        exit(1);
      }
      // se mi arriva come unico pkt un pkt di fine rapporto chiudo e scrivo sul
      // file però altrimenti non scrivo
      if (free_dim != 0) {
        int t = 0;
        for (t = 0; t < dim - free_dim; t++) {
          //printf("Indice %d - PL : %s\n", t, rcv_win[t].pl);
          if ((fwrite(rcv_win[t].pl, strlen(rcv_win[t].pl), 1, fptr) < 0)) {
            perror("Error in write rcv_get\n");
            exit(1);
          }
        }
      }
      stay = false;
      // se il finbit è 0 e il numero di pkt è quello che mi aspettavo scrivo
      // sul file il pkt ricevuto
    } else if (pkt.finbit == 0 && pkt.ack == n) {
      rcv_win[i] = pkt;
      i++;
      // cicliclo
      i = i % dim;
      different = false;
       free_dim--;
      printf(" finbit =  %d   ack = %d free_dim %d i = %d\n", pkt.finbit, pkt.ack,free_dim,i);
      fflush(stdout);
      // printf("SEQNUM %d \n NUM %d\n",pkt.ack,n);
      //  invio un ack ogni pkt che ricevo
        //printf(" ACK RCV %d ACK INV %d\n", pkt.ack, n);
      // fflush(stdout);
      pkt.ack = n;
      pkt.finbit = 0;
      pkt.rwnd = free_dim;
      if(free_dim == 0){
        strcpy(pkt.pl,"slow");
      }
      if (sendto(sockfd, &pkt, sizeof(pkt), 0, (struct sockaddr *)&servaddr,
                 addrlen) < 0) {
        perror("errore in sendto");
        exit(1);
      }
      if (free_dim == 0){
        int t = 0;
        free_dim=dim;
        for (t = 0; t < dim; t++) {
          if ((fwrite(rcv_win[t].pl, strlen(rcv_win[t].pl), 1, fptr) < 0)) {
            perror("Error in write rcv_get\n");
            exit(1);
          }
        }
      }
    }
    // se arriva un pkt fuori ordine invio subito ack non faccio la
    // bufferizzazione lato rcv
    else if (stay == true && pkt.ack != n) {
      // non incremento n pongo diff = true
      different = true;
      // invio al sender un ack comulativo fino a dove ho ricevuto
      //printf("ERR ACK RCV %d ACK INV %d\n", pkt.ack, n - 1);
      fflush(stdout);
      pkt.ack = n - 1;
      pkt.rwnd = free_dim;
      // gestire ack non in ordine ES: inviamo un ack al sender e gli diciamo di
      // inviare tutto dopo quel numero printf(" Client : Pkt fuori ordine
      // ricevuto invio ack [%d]\n",pkt.ack);
      if (sendto(sockfd, &pkt, sizeof(pkt), 0, (struct sockaddr *)&servaddr,
                 addrlen) < 0) {
        perror("errore in sendto");
        exit(1);
      }
    }
  }
  fclose(fptr);
  free(rcv_win);
}

// implemento la snd del comando put
void snd_put(char *str) {
  FILE *file;
  char snd_buff[MAXLINE + 2];
  char temp[MAXLINE];
  char rcv_buff[100];
  char path_file[200];
  char *fin = "END\n";
  bool stay = true;
  bool error = true;
  int t = 1;
  sprintf(path_file, "Server_Files/%s", str);
  printf("path %s\n", path_file);
  if ((file = fopen("ciao", "r+")) == NULL) {
    printf("Errore in open del file\n");
    exit(1);
  }
  while (stay) {
    while (fgets(temp, MAXLINE, file)) {
      // inserisco il numero al pkt che invio
      fflush(stdout);
      sprintf(snd_buff, "%d\n", t);
      sprintf(snd_buff + 2, "%s", temp);
      snd_buff[strlen(snd_buff)] = '\0';
      printf("PKT SIZE %ld\n", strlen(snd_buff));
      printf("invio il pkt %s", snd_buff);
      fflush(stdout);
      if ((sendto(sockfd, snd_buff, strlen(snd_buff), 0,
                  (struct sockaddr *)&servaddr, addrlen)) < 0) {
        perror("errore in sendto");
        exit(1);
      }
      t++;
    }
    if (feof(file)) {
      stay = false;
      printf("Chiusura comando put \n");

      printf("\nULTIMO PKT\n");

      fflush(stdout);

      // +2: perchè tolgo il numero
      // -2: perchè copio i bit ma devo togliere il numero  che vengono contati
      // dallo strlen
      printf("\nInvio al rcv il pkt\n %s\n", fin);

      fflush(stdout);
      if ((sendto(sockfd, fin, strlen(fin), 0, (struct sockaddr *)&servaddr,
                  addrlen)) < 0) {
        perror("errore in sendto");
        exit(1);
      }
      if (recvfrom(sockfd, rcv_buff, MAXLINE, 0, (struct sockaddr *)&servaddr,
                   &addrlen) < 0) {
        perror("errore in recvfrom");
        exit(1);
      }
      // implemento gestione fine connessione mi aspetto un pkt con solo OK dal
      // receiver
      printf("pkt ricevuto %s\n", rcv_buff);
      if (!strcmp(rcv_buff, "OK\n")) {
        printf("\nConnessione chiusa correttamente ricevuto ACK %s\n",
               rcv_buff);
      } else {
        // gestione errore ritrasmettere pkt
      }
    }
  }
  // controllo se è arrivato un ack dal receiver in caso gestisco gli errori non
  // bloccante
  /*if ((recvfrom(sockfd,rcv_buff,sizeof(rcv_buff),MSG_DONTWAIT, (struct
  sockaddr *)&addr,&addrlen)) < 0) { perror("errore in recvfrom"); exit(-1);
  }
  //ricevo qualcosa ancora da implementare
  if(strlen(rcv_buff) != 0){
          // se il numero ricevuto è diverso da quello corrente errore il
  receiver non bufferizza nulla rcv_number=strtol(rcv_buff,NULL,10);
          if(rcv_number != temp ){
                  while(error){

                          // qui devo implementare una variabile globale che
  mantiene i buff inviati almeno un po
                          // poi invio tutti i pkt dal rcv_number fino al temp e
  esco error=false;
                  }
          }
  }
*/
  // invio il pkt

  printf("\nSnd_put return\n");
}

// implemento la rcv del comando list
void rcv_list() {
  printf("rcv_list alive\n");
  char rcv_buff[MAXLINE];
  char snd_buff[30];
  char number_pkt[30];
  int n = 1, i = 0, k = 1;
  bool stay = true;
  while (stay) {
    snd_buff[0] = '\0';
    number_pkt[0] = '\0';
    rcv_buff[0] = '\0';
    if ((recvfrom(sockfd, rcv_buff, MAXLINE, 0, (struct sockaddr *)&servaddr,
                  &addrlen)) < 0) {
      perror("errore in recvfrom");
      exit(1);
    }
    while (rcv_buff[i] != '\n') {
      i++;
    }
    strncpy(number_pkt, rcv_buff, i);
    number_pkt[i] = '\0';
    // END == terminatore pkt inviati
    printf("NUM RICEVUTO-> %s\n", number_pkt);

    printf("NUM CHE VOGLIO->%d\n", n);
    if (!strcmp(number_pkt, "END")) {

      printf("Terminatore ricevuto\n");

      printf("%s\n", rcv_buff + i);
      fflush(stdout);
      // invio  ACK di fine ricezione al sender
      sprintf(snd_buff, "OK\n");

      if (sendto(sockfd, snd_buff, strlen(snd_buff), 0,
                 (struct sockaddr *)&servaddr, addrlen) < 0) {
        perror("errore in sendto");
        exit(1);
      }
      // fare sul server questa cosa |||||||||||||||
      stay = false;

      // confronto il numero ricevuto e quello che mi aspetto

    } else if (n == strtol(number_pkt, NULL, 10) && stay == true) {

      printf("valore di n->%d \n", n);

      printf("%s\n", rcv_buff + i);

      fflush(stdout);
      // incremento n
      n++;
    } else if (n != strtol(number_pkt, NULL, 10) && stay == true) {
      // gestire ack non in ordine ES: inviamo un ack al sender e gli diciamo di
      // inviare tutto dopo quel numero
      printf("Numero ricevuto diverso da quello che mi aspettavo\n");
      sprintf(snd_buff, "%d\n", n);
      if (sendto(sockfd, snd_buff, strlen(snd_buff), 0,
                 (struct sockaddr *)&servaddr, addrlen) < 0) {
        perror("errore in sendto");
        exit(1);
      }
    }
  }
}

// funzione che implementare la send to server
void command_send(char *cd, char *nome_str) {

  struct st_pkt pkt;
  int temp = 0, q = -2;
  pkt.ack = 0;
  char str[MAXLINE];
  strcpy(str, cd);
  if (nome_str != NULL) {
    strncpy(str + strlen(cd), nome_str, strlen(nome_str));
  }
  printf("\n nome comando %s\n", str);
  strcpy(pkt.pl, str);
  pkt.finbit = 0;
  int i = 0;
  // Invia al server il pacchetto di richiesta
  if (sendto(sockfd, &pkt, sizeof(pkt), 0, (struct sockaddr *)&servaddr,
             addrlen) < 0) {
    perror("errore in sendto");
    exit(1);
  }
  pkt.ack = -2;
  while (pkt.ack != 0 && pkt.ack != -1) {
    // Legge dal socket il pacchetto di risposta
    if (recvfrom(sockfd, &pkt, sizeof(pkt), 0, (struct sockaddr *)&servaddr,
                 &addrlen) < 0) {
      perror("errore in recvfrom");
      exit(1);
    }
  }
  printf("pkt.ack %d pkt.pl %s finbit %d \n", pkt.ack, pkt.pl, pkt.finbit);
  fflush(stdout);
  if (pkt.ack == -1) {
    printf("\n Error Server = %s\n", pkt.pl);
    fflush(stdout);
    req();
    loop = true;
    // vedere se va bene cosi con il return sennò facciamo altro
    return;
  } else if (pkt.ack == temp) {
    printf("\n Server response : %s\n", pkt.pl);
    fflush(stdout);
    // qui possiamo implementare la perdita del pkt
    // invio ack di conferma
    /*
    printf("\n Client: Send ack cum %d\n",pkt.ack);
    if (sendto(sockfd, &pkt, sizeof(pkt), 0,(struct sockaddr
*)&servaddr,addrlen) < 0) { perror("errore in sendto"); exit(1);
}
*/
    // implento la list
    if (!strcmp(cd, "list")) {
      rcv_list();
    }
    // implento la get
    else if (!strcmp(cd, "get ")) {
      fflush(stdout);
      rcv_get(nome_str);
    }
    // implemento la put
    else if (!strcmp(cd, "put ")) {
      snd_put(nome_str);
    }
  }
}
// gestisco la richiesta dell'utente
void req() {
  int a = 0, temp = 0;
  char buff[MAXLINE];
  while (1) {
    // se ho gestito un errore e si è creato un loop chiudo
    if (loop) {
      break;
    }

    printf("\nInserire numero:\nget = 0\nlist = 1\nput = 2\nexit = -1\n");

    fscanf(stdin, "%d", &a);

    switch (a) {
    case 0:
      printf("\nInserire nome file\n");
      if ((temp = read(0, buff, sizeof(buff))) < 0) {
        perror("Error fread");
        exit(1);
      }
      buff[temp - 1] = '\0';
      command_send("get ", buff);
      break;

    case 1:
      command_send("list",
                   NULL); // passo direttamento la list alla command_send senza
                          // usare una funzione ausiliaria
      break;

    case 2:
      printf("\nInserire nome file\n");
      if ((temp = read(0, buff, sizeof(buff))) < 0) {
        perror("Error fread");
        exit(1);
      }
      buff[temp - 1] = '\0';
      command_send("put ", buff);

      break;

    case -1:
      // chiusura
      return;
    }
  }
}

int main(int argc, char *argv[]) {
  char recvline[MAXLINE + 1];
  if (argc != 3) { // controlla numero degli argomenti
    fprintf(stderr, "utilizzo: <indirizzo IP server>  <num_pkt> \n");
    exit(1);
  }
  dim = atoi(argv[2]);
  free_dim=dim;
  if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) { // crea il socket
    perror("errore in socket");
    exit(1);
  }

  memset((void *)&servaddr, 0, sizeof(servaddr)); // azzera servaddr
                                                  //
  servaddr.sin_family = AF_INET;        // assegna il tipo di indirizzo
                                        //
  servaddr.sin_port = htons(SERV_PORT); // assegna la porta del server
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
