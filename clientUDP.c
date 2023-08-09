#include <arpa/inet.h>
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <pthread.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/syscall.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>
#include <wait.h>
#define SERV_PORT 5193
#define MAXLINE 4096
typedef void Sigfunc(int);

char pkt_send[MAXLINE];
bool loop = false;
int dim = 0;
int free_dim = 0;
int seqnum = 0;
int lt_ack_rcvd = 0;
int lt_rwnd = 1;
int swnd = 0;
int msgRitr = 0;
int CongWin = 1;
int TOms = 0;
int TOs = 0;
double p = 0;
long int bytes_psecond = 0;
bool rit = false;
int sockfd; // descrittore alla socket creata per comunicare con il server
struct sockaddr_in addr;
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
int port_number(int sockfd) {
  addr.sin_port = htons(SERV_PORT);
  struct st_pkt pkt;
  int temp = 0;
  pkt.finbit = 2;
  pkt.pl[0] = '\0';
  pkt.ack = 0;
  bool rcv_winy = true;
  printf("Invio pkt port\n");
  fflush(stdout);
  if (sendto(sockfd, &pkt, sizeof(pkt), 0, (struct addr *)&addr, addrlen) < 0) {
    perror("errore in sendto");
    exit(1);
  }
  while (pkt.ack < SERV_PORT) {
    if (recvfrom(sockfd, &pkt, sizeof(pkt), 0, (struct sockaddr *)&addr,
                 &addrlen) < 0) {
      perror("errore in recvfrom");
      exit(1);
    }
    printf("ack rcvd %d\n", pkt.ack);
    fflush(stdout);
  }
  printf("Client : pl %s port number %d\n", pkt.pl, pkt.ack);
  fflush(stdout);
  // se c'è wait ritrasmetto subito una nuova richiesta aspetto un tempo sleep
  if (!strcmp(pkt.pl, "wait")) {
    printf("Server response : wait\n");
    fflush(stdout);
    while (rcv_winy) {
      sleep(1);
      pkt.finbit = 2;
      if (temp ==
          5) { // dopo 5 volte di fila che provo a connettermi aspetto un bel po
        printf("Client : Impossibile connettersi con il Server attualmente \n");
        exit(1);
      }
      temp++;
      if (sendto(sockfd, &pkt, sizeof(pkt), 0, (struct sockaddr *)&addr,
                 addrlen) < 0) {
        perror("errore in sendto");
        exit(1);
      }
      if (recvfrom(sockfd, &pkt, sizeof(pkt), 0, (struct sockaddr *)&addr,
                   &addrlen) < 0) {
        perror("errore in recvfrom");
        exit(1);
      }
      if (!strcmp(pkt.pl, "go")) {
        rcv_winy = false;
      }
    }
  } else if (!strcmp(pkt.pl, "go")) {
    printf("Port number  assegnato %d \n", pkt.ack);
    return pkt.ack;
  }
}

// implemento la rcv del comando get
void rcv_get(char *file) {
  free_dim = dim;
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
    if ((recvfrom(sockfd, &pkt, sizeof(pkt), 0, (struct sockaddr *)&addr,
                  &addrlen)) < 0) {
      perror("errore in recvfrom");
      exit(1);
    }
    // printf("ack rcvd  %d\n\n ",pkt.ack);
    //  se mi arriva un ack che ho già salvato non lo mantengo nell'array
    //  finbit == 1 allora chiudo la connessione
    if (pkt.finbit == 1 && pkt.ack == n) {
      printf("OK ack = %d free_dim %d PL: %s \n", pkt.ack, free_dim, pkt.pl);
      fflush(stdout);
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
      // printf("\nACK --> %d N %d\n", pkt.ack,n);
      fflush(stdout);

      if (sendto(sockfd, &pkt, sizeof(pkt), 0, (struct sockaddr *)&addr,
                 addrlen) < 0) {
        perror("errore in sendto");
        exit(1);
      }
      // se mi arriva come unico pkt un pkt di fine rapporto chiudo e scrivo sul
      // file però altrimenti non scrivo
      if (free_dim != 0) {
        int t = 0;
        for (t = 0; t < dim - free_dim; t++) {
          // printf("Indice %d - PL : %s\n", t, rcv_win[t].pl);
          fflush(stdout);
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
      printf("OK ack = %d free_dim %d\n", pkt.ack, free_dim);
      fflush(stdout);
      pkt.ack = n;
      pkt.finbit = 0;
      pkt.rwnd = free_dim;
      if (free_dim == 0) {
        printf("Client send slow to Server\n");
        fflush(stdout);
        strcpy(pkt.pl, "slow");
        pkt.rwnd = dim; // scrivo tutto sul file allora mando come nuova rwnd la
                        // dim(il server rallenta la send)
      }
      if (sendto(sockfd, &pkt, sizeof(pkt), 0, (struct sockaddr *)&addr,
                 addrlen) < 0) {
        perror("errore in sendto");
        exit(1);
      }
      if (free_dim == 0) {
        int t = 0;
        free_dim = dim;
        for (t = 0; t < dim; t++) {
          if ((fwrite(rcv_win[t].pl, strlen(rcv_win[t].pl), 1, fptr) < 0)) {
            perror("Error in write rcv_get\n");
            exit(1);
          }
        }
        printf("Client write all pkt and send msg to Server\n");
        fflush(stdout);
      }
    }
    // se arriva un pkt fuori ordine invio subito ack non faccio la
    // bufferizzazione lato rcv
    else if (stay == true && pkt.ack != n) {
      // non incremento n pongo diff = true
      different = true;
      // invio al sender un ack comulativo fino a dove ho ricevuto
      printf("NO num %d  invio ack %d \n", pkt.ack, n - 1);
      fflush(stdout);
      pkt.ack = n - 1;
      pkt.rwnd = free_dim;
      if (sendto(sockfd, &pkt, sizeof(pkt), 0, (struct sockaddr *)&addr,
                 addrlen) < 0) {
        perror("errore in sendto");
        exit(1);
      }
    }
  }
  fclose(fptr);
  free(rcv_win);
}
void *rcv_cong(void *sd) {
  int sockfd = sd;
  struct st_pkt pkt;
  int k = 0, temp = 0, n;
  lt_ack_rcvd = 0;
  // Wait until timeout or data received.
  bool stay = true;
  while (stay) {
    if (lt_ack_rcvd == k) {
      printf("pkt %d start TO\n", pkt.ack);
      alarm(2);
    }
    if (recvfrom(sockfd, &pkt, sizeof(pkt), 0, (struct sockaddr *)&addr,
                 &addrlen) < 0) {
      perror("errore in recvfrom");
      exit(1);
    }
    printf("RCV_CONG : rcvd ack %d \n", pkt.ack);
    if (pkt.ack > lt_ack_rcvd) {
      alarm(0);
      lt_ack_rcvd = pkt.ack;
      k = lt_ack_rcvd;
      CongWin++;
      swnd = seqnum - lt_ack_rcvd;
      lt_rwnd = pkt.rwnd;
      lt_ack_rcvd = pkt.ack;
      // se è un ack nuovo entro qui
      //  se non ho errore allora leggo e vedo l ack che il receiver mi invia
      if (!strcmp(pkt.pl, "slow")) {
        printf("\nSLOW RCVD\n");
        fflush(stdout);
        bytes_psecond = 100;
        if (setsockopt(sockfd, SOL_SOCKET, SO_MAX_PACING_RATE, &bytes_psecond,
                       sizeof(bytes_psecond)) < 0) {
          perror("Error setsockopt");
          exit(1);
        }
      } else {
        bytes_psecond = 1000;
        if (setsockopt(sockfd, SOL_SOCKET, SO_MAX_PACING_RATE, &bytes_psecond,
                       sizeof(bytes_psecond)) < 0) {
          perror("Error setsockopt");
          exit(1);
        }
      }
      if (pkt.finbit == 1 && pkt.ack == seqnum) {
        printf("Server : Client disconesso  correttamente \n");
        fflush(stdout);
        stay = false;
        return NULL;
      }
    }
  }
}
// implemento la snd del comando put
void snd_put(char *str, int sockfd) {
  // variabili da resettare
  seqnum = 0;
  lt_ack_rcvd = 0;
  swnd = 0;
  CongWin = 1;
  dim = 0;
  lt_rwnd = 1;
  int size = 0;
  printf("\nSend_put\n");
  fflush(stdout);
  struct st_pkt pkt;
  FILE *file;
  bool stay = true;
  int i = 0, msgInviati = 0, msgPerso = 0, msgTot = 0, dimpl = 0;
  double prob = 0;
  char path_file[MAXLINE];
  sprintf(path_file, "Client_Files/%s", str);
  if ((file = fopen(path_file, "r+")) == NULL) {
    printf("Errore in open del file\n");
    exit(1);
  }
  // ottengo la dimesione del file cosi da definire la dim
  fseek(file, 0, SEEK_END);     // seek to end of file
  size = ftell(file);           // get current file pointer
  fseek(file, 0, SEEK_SET);     // seek back to beginning of file
  dim = ((size) / MAXLINE) + 1; // +1 perchè arrotonda per difetto
  if ((rcv_win = malloc(sizeof(struct st_pkt) * dim)) == NULL) {
    perror("Error malloc");
    exit(1);
  }
  printf("Client : Send_list Alive\n");
  pthread_t thread_id;
  // creo il thread che mi legge le socket (lettura bloccante)
  if (pthread_create(&thread_id, NULL, rcv_cong, sockfd) != 0) {
    perror("error pthread_create");
    exit(1);
  }
  while (stay) {
    // se last ack non è uguale al mio seqnum mi fermo altrimenti entro dentro e
    // invio da 0 a CongWin pkt e poi mi aspetto di ricevere come lastack quello
    // dell'ultimo pkt inviato poi continuo
    if (lt_ack_rcvd == seqnum && stay == true && !rit) {
      while (swnd < CongWin && stay == true && swnd < lt_rwnd && !rit) {
        printf(" SEND_PUT :: swnd = %d CongWin = %d  lt_rwnd = %d\n", swnd,
               CongWin, lt_rwnd);
        fflush(stdout);
        if ((dimpl = fread(pkt.pl, 1, sizeof(pkt.pl), file)) == MAXLINE) {
          seqnum++;
          swnd++;
          pkt.finbit = 0;
          pkt.ack = seqnum;
          // mantengo CongWin pkt
          rcv_win[i] = pkt;
          i++;
          // cicliclo
          i = i % dim;
          // aumento la dim del vettore che mi salva i pkt
          prob = (double)rand() / RAND_MAX;
          if (prob < p) {
            msgPerso++;
            msgTot++;
            swnd++;
            // Il messaggio è stato perso
            continue;
          } else {
            // Trasmetto con successo
            if (lt_ack_rcvd < seqnum - 5) {
              // rallento flow control
              bytes_psecond = 10;
              if (setsockopt(sockfd, SOL_SOCKET, SO_MAX_PACING_RATE,
                             &bytes_psecond, sizeof(bytes_psecond)) < 0) {
                perror("Error setsockopt");
                exit(1);
              }
            } else {
              bytes_psecond = 100;
              if (setsockopt(sockfd, SOL_SOCKET, SO_MAX_PACING_RATE,
                             &bytes_psecond, sizeof(bytes_psecond)) < 0) {
                perror("Error setsockopt");
                exit(1);
              }
            }
            if ((sendto(sockfd, &pkt, sizeof(pkt), 0, (struct sockaddr *)&addr,
                        addrlen)) < 0) {
              perror("errore in sendto");
              exit(1);
            }
            msgInviati++;
            msgTot++;
          }
          // la lettura la fa il thread cosi non mi blocco io main thread
        } else if (feof(file)) {
          while (lt_ack_rcvd != seqnum) {
            // Aspetto che il thread legga last ack
            usleep(5);
          }
          printf("fine file\n");
          fflush(stdout);
          seqnum++;
          pkt.ack = seqnum;
          pkt.finbit = 1;
          pkt.pl[dimpl] = '\0';
          rcv_win[i] = pkt;
          stay = false;
          swnd++;
          printf("\n\n\nSERVER : send last pkt %d %s  ", seqnum, pkt.pl);
          prob = (double)rand() / RAND_MAX;
          printf("\n Server : Close connection \n");
          fflush(stdout);
          if (prob < p) {
            // Il messaggio è stato perso
            msgPerso++;
            msgTot++;
          } else {
            // invio il terminatore
            if (sendto(sockfd, &pkt, sizeof(pkt), 0, (struct sockaddr *)&addr,
                       addrlen) < 0) {
              perror("Error in sendto");
              exit(1);
            }
            msgInviati++;
            msgTot++;
          }
        } else if (ferror(file)) {
          perror("Error fread ");
          exit(1);
        }
      }
    }
  }
  // aspetto la terminazione del thread che legge
  if (pthread_join(thread_id, NULL) != 0) {
    perror("Error pthread_join");
    exit(1);
  }
  printf("\nMSG TOTALI %d\n", msgTot);
  printf("\nMSG PERSI %d\n", msgPerso);
  printf("\nMSG INVIATI %d\n", msgInviati);
  printf("\n Dim CongWin finale %d\n", CongWin);
  printf("\n PROB DI SCARTARE UN MSG %f\n", p);
  fflush(stdout);
  free(rcv_win);
  fclose(file);
}

// implemento la rcv del comando list
void rcv_list() {
  free_dim = dim;
  struct st_pkt pkt;
  if ((rcv_win = malloc(sizeof(struct st_pkt) * dim)) == NULL) {
    perror("Error malloc");
    exit(1);
  }
  printf("\n Client : List function alive\n");
  fflush(stdout);
  int n = 0, i = 0, maxseqnum = 0, k = 0;
  bool stay = true, different = false;
  while (stay) {
    // incremento n
    if (!different) {
      n++;
    }
    if ((recvfrom(sockfd, &pkt, sizeof(pkt), 0, (struct sockaddr *)&addr,
                  &addrlen)) < 0) {
      perror("errore in recvfrom");
      exit(1);
    }
    // se mi arriva un ack che ho già salvato non lo mantengo nell'array
    // finbit == 1 allora chiudo la connessione
    if (pkt.finbit == 1 && pkt.ack == n) {
      printf("OK ack = %d free_dim %d PL: %s \n", pkt.ack, free_dim, pkt.pl);
      fflush(stdout);
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
      fflush(stdout);
      if (sendto(sockfd, &pkt, sizeof(pkt), 0, (struct sockaddr *)&addr,
                 addrlen) < 0) {
        perror("errore in sendto");
        exit(1);
      }
      printf("Client : List -> ::\n ");
      if (free_dim != 0) {
        int t = 0;
        for (t = 0; t < dim - free_dim; t++) {
          printf("%s\n", rcv_win[t].pl);
        }
        puts("\n");
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
      printf("OK ack = %d free_dim %d PL: %s\n", pkt.ack, free_dim, pkt.pl);
      fflush(stdout);
      pkt.ack = n;
      pkt.finbit = 0;
      pkt.rwnd = free_dim;
      if (free_dim == 0) {
        printf("Client send slow to Server\n");
        fflush(stdout);
        strcpy(pkt.pl, "slow");
        pkt.rwnd = dim; // scrivo tutto sul file allora mando come nuova rwnd la
                        // dim(il server rallenta la send)
      }
      if (sendto(sockfd, &pkt, sizeof(pkt), 0, (struct sockaddr *)&addr,
                 addrlen) < 0) {
        perror("errore in sendto");
        exit(1);
      }
      // se rcv_win è pieno stampo a schermo
      if (free_dim == 0) {
        k = 0;
        while (k < dim) {
          printf("%s\n", rcv_win[k].pl);
          k++;
        }
      }
    }
    // se arriva un pkt fuori ordine invio subito ack non faccio la
    // bufferizzazione lato rcv
    else if (stay == true && pkt.ack != n) {
      // non incremento n pongo diff = true
      different = true;
      // invio al sender un ack comulativo fino a dove ho ricevuto
      pkt.ack = n - 1;
      pkt.rwnd = free_dim;
      if (sendto(sockfd, &pkt, sizeof(pkt), 0, (struct sockaddr *)&addr,
                 addrlen) < 0) {
        perror("errore in sendto");
        exit(1);
      }
    }
  }
  free(rcv_win);
}

// funzione che implementare la send to server
void command_send(char *cd, char *nome_str) {
  struct st_pkt pkt;
  int temp = 0;
  pkt.ack = 0;
  char str[MAXLINE];
  strcpy(str, cd);
  if (nome_str != NULL) {
    strcat(str + strlen(cd), nome_str);
  }
  printf("\n nome comando %s\n", str);
  fflush(stdout);
  strcpy(pkt.pl, str);
  pkt.finbit = 0;
  int i = 0;
  // Invia al server il pacchetto di richiesta
  if (sendto(sockfd, &pkt, sizeof(pkt), 0, (struct sockaddr *)&addr, addrlen) <
      0) {
    perror("errore in sendto");
    exit(1);
  }
  pkt.ack = -2;
  while (pkt.ack != 0 && pkt.ack != -1) {
    // Legge dal socket il pacchetto di risposta
    if (recvfrom(sockfd, &pkt, sizeof(pkt), 0, (struct sockaddr *)&addr,
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
      snd_put(nome_str, sockfd);
    }
  }
  // leggo tutti i byte nella socket mi serve nel caso in cui il client vuole
  // comunicare nuovamente con la stessa socket

  int len = 0, u = 0;
  bool r = true;
  char buffer[MAXLINE];

  while (r) { // leggo tutto quello che è rimasto nella socket gestisco
              // eventuali ritrasmissioni ritardate di pkt
    if (u = read(sockfd, buffer, len) == -1) {
      perror("Error read");
      exit(1);
    }
    if (u == 0) {
      r = false;
      continue;
    }
  }
}
// gestisco la richiesta dell'utente
void req() {
  struct st_pkt pkt;
  pkt.ack = 0;
  pkt.finbit = 0;
  int a = 0, temp = 0, t = 0;
  while (1) {
    a = 0, temp = 0, t = 0;
    // se ho gestito un errore e si è creato un loop chiudo
    if (loop) {
      break;
    }
    if ((recvfrom(sockfd, &pkt, sizeof(pkt), MSG_DONTWAIT,
                  (struct sockaddr *)&addr, &addrlen)) < 0) {
      if (errno == EAGAIN) {

      } else {
        perror("errore in recvfrom");
        exit(-1);
      }
    }
    if (pkt.finbit == 3 && !strcmp(pkt.pl, "Close")) {
      printf("Client : Server close connection   \n");
      temp = port_number(sockfd);
      printf("port number %d\n", temp);
      fflush(stdout);
      addr.sin_port = htons(temp); // assegna la porta presa dal server
    } else {
      temp = port_number(sockfd);
      printf("port number %d\n", temp);
      fflush(stdout);
      addr.sin_port = htons(temp); // assegna la porta presa dal server
    }
    printf("\nInserire numero:\nget = 0\nlist = 1\nput = 2\nexit = -1\n");
    if (fscanf(stdin, "%d", &a) == EOF) {
      perror("Error fscanf");
      exit(1);
    }
    if (a == -1) { // chiudo subito senza che invio il pkt al server
      return;
    }
    // modificare la porta della sockfd
    addr.sin_port = htons(SERV_PORT);
    if ((recvfrom(sockfd, &pkt, sizeof(pkt), MSG_DONTWAIT,
                  (struct sockaddr *)&addr, &addrlen)) < 0) {
      if (errno == EAGAIN) {
      } else {
        perror("errore in recvfrom");
        exit(-1);
      }
    }
    if (pkt.finbit == 3 && !strcmp(pkt.pl, "Close")) {
      printf("Client : Server close connection last retry  \n");
      temp = port_number(sockfd);
      printf("port number %d\n", temp);
      fflush(stdout);
      addr.sin_port = htons(temp); // assegna la porta presa dal server
    } else {
      addr.sin_port = htons(temp);
    }
    // gestisco il caso in cui il client inserisce un numero diverso da quello
    // desiderato
    while (a != 0 && a != 1 && a != 2 && a != -1) {
      t++;
      printf("\nInserire numero:\nget = 0\nlist = 1\nput = 2\nexit = -1\n");
      fscanf(stdin, "%d", &a);
      fflush(stdout);
      if (t > 5) {
        exit(1);
      }
    }
    char buff[MAXLINE];
    switch (a) {
    case 0:
      printf("\nInserire dimensione buff\n");
      if (fscanf(stdin, "%d", &dim) == EOF) {
        perror("Error fscanf");
        exit(1);
      }
      free_dim = dim;
      printf("\nInserire nome file\n");
      if ((temp = read(0, buff, sizeof(buff))) < 0) {
        perror("Error fread");
        exit(1);
      }
      buff[temp - 1] = '\0';
      command_send("get ", buff);
      break;

    case 1:
      printf("\nInserire dimensione buff\n");
      if (fscanf(stdin, "%d", &dim) == EOF) {
        perror("Error fscanf");
        exit(1);
      }
      free_dim = dim;
      command_send("list",
                   NULL); // passo direttamento la list alla command_send senza
                          // usare una funzione ausiliaria
      break;

    case 2:
      system("mkdir Client_Files");
      printf("\nInserire TO in ms\n");
      if (fscanf(stdin, "%d", &TOms) == EOF) {
        perror("Error fscanf");
        exit(1);
      }
      printf("\nInserire TO in s\n");
      if (fscanf(stdin, "%d", &TOs) == EOF) {
        perror("Error fscanf");
        exit(1);
      }
      printf("\nInserire p \n");
      if (fscanf(stdin, "%le", &p) == EOF) {
        perror("Error fscanf");
        exit(1);
      }

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
// blocco i segnali esterni tranne alcuni che decido ctrl
Sigfunc *signal(int signum, Sigfunc *func) {
  struct sigaction act, oact;
  /* la struttura sigaction memorizza informazioni riguardanti la
  manipolazione del segnale */

  act.sa_handler = func;
  sigemptyset(&act.sa_mask); /* non occorre bloccare nessun altro segnale */
  act.sa_flags |= SA_RESTART;
  if (sigaction(signum, &act, &oact) < 0)
    return (SIG_ERR);
  return (oact.sa_handler);
}

void sig_int(int signo) { exit(1); }
void sig_time(int signo) {
  rit = true;
  printf("TO pkt %d finish \n", lt_ack_rcvd + 1);
  // concorrente con rcv_cong per ricevere il pkt nella socket sistemare
  struct st_pkt pkt;
  int k;
  if (CongWin > 1) {
    CongWin = CongWin >> 2;
    swnd = 0;
  }
  k = lt_ack_rcvd;
  //  implemento la ritrasmissione di tutti i pkt dopo lt_ack_rcvd
  while (k < seqnum) {
    // printf("k value %d seqnum value %d swnd value %d Congwin value %d lt_rwnd
    // = %d \n", k, num, swnd, CongWin,lt_rwnd); fflush(stdout);
    while (swnd < CongWin && swnd < lt_rwnd && k < seqnum) {
      printf("Sig_time : send pkt %d k = %d\n", rcv_win[k].ack, k);
      fflush(stdout);
      if ((sendto(sockfd, &rcv_win[k], sizeof(pkt), 0, (struct sockaddr *)&addr,
                  addrlen)) < 0) {
        perror("errore in sendto");
        exit(1);
      }
      msgRitr++;
      k++;
      swnd++;
    }
  }
  rit = false;
}

int main(int argc, char *argv[]) {
  char recvline[MAXLINE + 1];
  int temp = 0;
  if (argc != 2) { // controlla numero degli argomenti
    fprintf(stderr, "utilizzo: <indirizzo IP server>\n");
    exit(1);
  }
  if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) { // crea il socket
    perror("errore in socket");
    exit(1);
  }
  memset((void *)&addr, 0, sizeof(addr)); // azzera addr
                                          //
  addr.sin_family = AF_INET;              // assegna il tipo di indirizzo
  addr.sin_port = htons(SERV_PORT);       //
                                          //
  // assegna l'indirizzo del server prendendolo dalla riga di comando.
  // L'indirizzo è una stringa da convertire in intero secondo network byte
  // order.
  if (inet_pton(AF_INET, argv[1], &addr.sin_addr) <= 0) {
    // inet_pton (p=presentation) vale anche per indirizzi IPv6
    fprintf(stderr, "errore in inet_pton per %s", argv[1]);
    exit(1);
  }
  if (signal(SIGINT, sig_int) == SIG_ERR) {
    fprintf(stderr, "errore in signal INT ");
    exit(1);
  }
  if (signal(SIGALRM, sig_time) == SIG_ERR) {
    fprintf(stderr, "errore in signal INT ");
    exit(1);
  }
  // invoco la funzione per gestire le richieste dell'utente
  req();
}
