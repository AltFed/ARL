#include <arpa/inet.h>
#include <asm-generic/errno-base.h>
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
#include <sys/mman.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/syscall.h>
#include <sys/time.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>
#include <wait.h>
#define MAXLINE 4096
// problema quando perdo un pkt e swnd = Conwing - 1 quindi non trasmetto un pkt
// successivo e quindi non mi arriva un ack duplicato non mi arriva proprio l
// ack
//  variabili globali ORA NO SENTO UN 0.1SECONDI DI VOCE OGNI 2 SECONDI
int seqnum = 0;
int timeout = 0;
int lt_ack_rcvd = 0;
int lt_rwnd = 1;
int swnd = 0;
int CongWin = 1;
bool problem;
double p = 0;
int dim = 10;
int free_dim = 0;
int fd;
int msgRitr = 0;
long int bytes_psecond = 0;
bool *stop;
int SERV_PORT;
int nchildren;
pid_t *pids;
struct sockaddr_in addr;
struct sockaddr_in addrmain;
socklen_t addrlen = sizeof(struct sockaddr_in);
bool rit = false;
int num = 0;
bool w = false;
int fast_rit = 0;
// implementa il controllo di segnali per gestire i thread
typedef void Sigfunc(int);

// pkt struct
struct st_pkt {
  int ack;
  int finbit;
  char pl[MAXLINE];
  int rwnd;
};
// array per mantenere i pkt
struct st_pkt *retr;
void *mretr() {
  bool s = true;
  while (s) {
    usleep(timeout);
    if (w) {
      w = false;
      rit = true;
      printf("MRETR\n\n");
      fflush(stdout);
      struct st_pkt pkt;
      int k;
      if (CongWin > 1) {
        CongWin = CongWin >> 1;
      }
      swnd = 0;
      k = lt_ack_rcvd;
      printf("k = %d seqnum = %d \n", k, seqnum);
      fflush(stdout);
      //  implemento la ritrasmissione di tutti i pkt dopo lt_ack_rcvd

      while (k < seqnum) {
        sleep(1);
        puts("ciuao");
        while (swnd < CongWin && swnd < lt_rwnd && k < seqnum) {
          printf("MRETR : send pkt %d k = %d swnd %d Conwing %d seqnum %d "
                 "lt_rwnd %d \n",
                 retr[k].ack, k, swnd, CongWin, seqnum, lt_rwnd);
          fflush(stdout);
          if ((sendto(fd, &retr[k], sizeof(pkt), 0, (struct sockaddr *)&addr,
                      addrlen)) < 0) {
            perror("errore in sendto");
            exit(1);
          }
          swnd++;
          msgRitr++;
          k++;
        }
      }
      if (k == dim) {
        s = false; // prova poi si cambia
      }
      printf("swnd value %d \n", swnd);
      fflush(stdout);
      rit = false;
    }
  }
}

// thread per la gestione del ack cum
void *rcv_cong(void *sd) {
  int sockfd = sd;
  fd = sd;
  struct st_pkt pkt;
  int t = 0;
  pthread_t thread_id;
  int temp = 0, n;
  lt_ack_rcvd = 0;
  // Wait until timeout or data received.
  bool stay = true;
  if (pthread_create(&thread_id, NULL, mretr, NULL) != 0) {
    perror("error pthread_create");
    exit(1);
  }
  while (stay) {
    // puts("prima");
  rcv:
    if (recvfrom(sockfd, &pkt, sizeof(pkt), 0, (struct sockaddr *)&addr,
                 &addrlen) < 0) {
      perror("errore in recvfrom");
      if (errno = EINTR)
        goto rcv;
      exit(1);
    }
    printf("RCV_CONG : rcvd ack %d lt_rcvd %d\n", pkt.ack, lt_ack_rcvd);
    if (pkt.ack > lt_ack_rcvd) {
      w = false;
      printf("timer pkt %d\n", pkt.ack + 1);
      num = pkt.ack + 1;
      lt_ack_rcvd = pkt.ack;
      fflush(stdout);
      // ogni volta che ricevo un ack nuovo avvio il timer
      CongWin++;
      swnd = seqnum-lt_ack_rcvd;
      lt_rwnd = pkt.rwnd;
      // puts("CIAO1");
      // se è un ack nuovo entro qui
      //  se non ho errore allora leggo e vedo l ack che il receiver mi invia
      /*if (!strcmp(pkt.pl, "slow")) {
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
      }*/
      // puts("CIAO");
      if (pkt.finbit == 1 && pkt.ack == seqnum) {
        printf("Server : Client disconesso  correttamente \n");
        fflush(stdout);
        stay = false;
        return NULL;
      }
    } else {
      w = true;
    }
  } // aspetto la terminazione del thread che legge
  if (pthread_join(thread_id, NULL) != 0) {
    perror("Error pthread_join");
    exit(1);
  }
  printf("RCV_CONG EXIT\n");
  fflush(stdout);
}
// gestice nello specifico il comando get
void send_get(char *str, int sockfd) {
  // variabili da resettare
  seqnum = 0;
  lt_ack_rcvd = 0;
  swnd = 0;
  CongWin = 1;
  int size = 0;
  lt_rwnd = 1;
  printf("\nSend_get\n");
  fflush(stdout);
  struct st_pkt pkt;
  FILE *file;
  bool stay = true;
  int i = 0, msgInviati = 0, msgPerso = 0, msgTot = 0, dimpl = 0;
  double prob = 0;
  char path_file[MAXLINE];
  sprintf(path_file, "Server_Files/%s", str);
  if ((file = fopen(path_file, "r+")) == NULL) {
    printf("Errore in open del file\n");
    exit(1);
  }
  // ottengo la dimesione del file cosi da definire la dim
  fseek(file, 0, SEEK_END);     // seek to end of file
  size = ftell(file);           // get current file pointer
  fseek(file, 0, SEEK_SET);     // seek back to beginning of file
  dim = ((size) / MAXLINE) + 1; // +1 perchè arrotonda per difetto
  if ((retr = malloc(sizeof(struct st_pkt) * dim)) == NULL) {
    perror("Error malloc");
    exit(1);
  }
  printf("\ndim %d\n", dim);
  pthread_t thread_id;
  // creo il thread che mi legge le socket (lettura bloccante)
  if (pthread_create(&thread_id, NULL, rcv_cong, sockfd) != 0) {
    perror("error pthread_create");
    exit(1);
  }
  while (stay) {
    while (swnd < CongWin && stay == true && swnd < lt_rwnd && !rit) {
      if ((dimpl = fread(pkt.pl, 1, sizeof(pkt.pl), file)) == MAXLINE) {
        seqnum++;
        swnd++;
        pkt.finbit = 0;
        pkt.ack = seqnum;
        // mantengo CongWin pkt
        retr[i] = pkt;
        i++;
        // cicliclo
        i = i % dim;
        // aumento la dim del vettore che mi salva i pkt
        prob = (double)rand() / RAND_MAX;
        if (prob < p) {
          // printf(" SEND_GET ::  lost ACK = %d  swnd = %d CongWin = %d lt_rwnd
          // = %d\n", pkt.ack,swnd,CongWin, lt_rwnd);
          msgPerso++;
          msgTot++;
          // Il messaggio è stato perso
          printf(" LOST :: ACK = %d  swnd = %d CongWin = %d  lt_rwnd = %d\n",
                 pkt.ack, swnd, CongWin, lt_rwnd);
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
          printf("SEND_GET :: ACK = %d  swnd = %d CongWin = %d  lt_rwnd = %d\n",
                 pkt.ack, swnd, CongWin, lt_rwnd);
          fflush(stdout);
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
        }
        printf("fine file\n");
        fflush(stdout);
        seqnum++;
        swnd++;
        pkt.ack = seqnum;
        pkt.finbit = 1;
        pkt.pl[dimpl] = '\0';
        retr[i] = pkt;
        stay = false;
        printf("\n\n\nSERVER : send last pkt %d %s  ", seqnum, pkt.pl);
        prob = (double)rand() / RAND_MAX;
        printf("\n Server : Close connection \n");
        fflush(stdout);
        if (prob < p) {
          // Il messaggio è stato perso
          msgPerso++;
          msgTot++;
          continue;
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
  // aspetto la terminazione del thread che legge
  if (pthread_join(thread_id, NULL) != 0) {
    perror("Error pthread_join");
    exit(1);
  }
  printf("\nMSG TOTALI %d\n", msgTot);
  printf("\nMSG PERSI %d\n", msgPerso);
  printf("\nMSG INVIATI %d\n", msgInviati);
  printf("\n Dim CongWin finale %d\n", CongWin);
  printf("\n Swnd finale : %d\n",swnd);
  printf("\n PROB DI SCARTARE UN MSG %f\n", p);
  fflush(stdout);
  free(retr);
  fclose(file);
}
// gestice nello specifico il comando put
void rcv_put(char *file, int sockfd) {
  dim = 100;
  free_dim = dim;
  struct st_pkt pkt;
  if ((retr = malloc(sizeof(struct st_pkt) * dim)) == NULL) {
    perror("Error malloc");
    exit(1);
  }
  printf("\n Server : Put function alive\n");
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
      printf("OK ack = %d free_dim %d\n", pkt.ack, free_dim);
      fflush(stdout);
      retr[i] = pkt;
      i++;
      // cicliclo
      i = i % dim;
      free_dim--;
      printf("\n Server : Client close connection \n");
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
          if ((fwrite(retr[t].pl, strlen(retr[t].pl), 1, fptr) < 0)) {
            perror("Error in write rcv_get\n");
            exit(1);
          }
        }
      }
      stay = false;
      // se il finbit è 0 e il numero di pkt è quello che mi aspettavo scrivo
      // sul file il pkt ricevuto
    } else if (pkt.finbit == 0 && pkt.ack == n) {
      retr[i] = pkt;
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
          if ((fwrite(retr[t].pl, strlen(retr[t].pl), 1, fptr) < 0)) {
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
  free(retr);
}

// gestice nello specifico il comando list
void send_list(int sockfd) {
  seqnum = 0;
  lt_ack_rcvd = 0;
  swnd = 0;
  CongWin = 1;
  lt_rwnd = 1;
  dim = 100;
  struct st_pkt pkt;
  DIR *directory;
  struct dirent *file;
  bool stay = true;
  int i = 0, msgInviati = 0, msgPerso = 0, msgTot = 0, dimpl = 0, k = 0,
      temp = 0;
  double prob = 0;
  int y = 10;
  pthread_t thread_id;
  char **nomi = malloc(sizeof(char *) * y);
  int c = 1;

  if ((retr = malloc(sizeof(struct st_pkt) * dim)) == NULL) {
    perror("Error malloc");
    exit(1);
  }
  printf("Server : Send_list Alive\n");
  printf("swnd %d lt %d seq %d rwnd %d congwin %d\n", swnd, lt_ack_rcvd, seqnum,
         lt_rwnd, CongWin);
  fflush(stdout);
  // creo il thread che mi legge le socket
  if (pthread_create(&thread_id, NULL, rcv_cong, sockfd) != 0) {
    perror("error pthread_create");
    exit(1);
  }
  // Apertura della cartella
  directory = opendir("Server_Files");

  // Verifico se la cartella è stata aperta correttamente
  if (directory == NULL) {
    printf("Impossibile aprire la cartella.\n");
    exit(1);
  }
  while ((file = readdir(directory)) != NULL) {
    if (strcmp(file->d_name, ".") != 0 && strcmp(file->d_name, "..") != 0) {
      if ((nomi[c - 1] = malloc(strlen(file->d_name) + 1)) == NULL) {
        perror("error malloc ");
        exit(1);
      }
      strncpy(nomi[c - 1], file->d_name, strlen(file->d_name) + 1);
      if (c % y == 0) {
        y = y << 1;
        printf("%d\n", y * sizeof(char *));
        if ((nomi = realloc(nomi, y * sizeof(char *))) == NULL) {
          perror("error realloc");
          exit(1);
        }
        printf("%ld\n", nomi);
      }
      c++;
    }
  }
  if (!file) {
    c--;
  }
  for (int x = 0; x < (c - 1); x++) {
    printf("----->>> %s\n", nomi[x]);
    fflush(stdout);
  }
  if (c == 1) {
    puts("nessun file nella dir\n");

    pkt.ack = -1;
    strcpy(pkt.pl, "nessun file in dir");
    if ((sendto(sockfd, &pkt, sizeof(pkt), 0, (struct sockaddr *)&addr,
                addrlen)) < 0) {
      perror("errore in sendto");
      exit(1);
    }

    return;
  }
  while (stay) {
    if (lt_ack_rcvd == seqnum && stay == true && !rit) {
      // Lettura dei file all'interno della cartella

      while (c > 0 && swnd < CongWin && stay == true && swnd < lt_rwnd &&
             !rit) {
        // memorizzo i nomi
        printf("c value %d , swnd = %d , Congwin= %d,lt_rwnd %d  stay = %d rit "
               "=%d \n",
               c, swnd, CongWin, lt_rwnd, stay, rit);
        fflush(stdout);
        temp = strlen(nomi[c - 1]);
        strncpy(pkt.pl, nomi[c - 1], temp);
        printf("pkt.pl %s \n", nomi[c - 1]);
        fflush(stdout);
        pkt.pl[temp] = '\0'; // risolvo il problema di scrivere sul buff pieno
        swnd++;
        seqnum++;
        c--; // decremento
        pkt.ack = seqnum;
        pkt.finbit = 0;
        retr[i] = pkt;
        i++;
        i = i % dim;
        prob = (double)rand() / RAND_MAX;
        if (prob < p) {
          msgPerso++;
          msgTot++;
        } else {
          if ((sendto(sockfd, &pkt, sizeof(pkt), 0, (struct sockaddr *)&addr,
                      addrlen)) < 0) {
            perror("errore in sendto");
            exit(1);
          }
          msgInviati++;
          msgTot++;
        }
      }
      // mando l'ultimo pkt
      if (c == 0) {
        while (lt_ack_rcvd != seqnum) {
          // Aspetto che il thread legga last ack
        }
        swnd++;
        // Invio l'ultimo pkt chiudo la connessione
        stay = false;
        pkt.finbit = 1;
        seqnum++;
        pkt.ack = seqnum;
        pkt.pl[0] = '\0';
        retr[i] = pkt;
        prob = (double)rand() / RAND_MAX;
        printf("Server : Chiusura connessione\n");
        if (prob < p) {
          msgPerso++;
          msgTot++;
        } else {
          if ((sendto(sockfd, &pkt, sizeof(pkt), 0, (struct sockaddr *)&addr,
                      addrlen)) < 0) {
            perror("errore in sendto");
            exit(1);
          }
          msgInviati++;
          msgTot++;
        }
      }
    }
  }
  // aspetto terminazione del thread
  if (pthread_join(thread_id, NULL) != 0) {
    perror("Error pthread_join");
    exit(1);
  }
  printf("Server : Connessione terminata corretamente\n");
  // Chiusura della cartella
  closedir(directory);
  printf("Server : Chiudo la directory\n");
  free(retr);
}
// gestisce il comando che il client richiede
void send_control(int sockfd, int my_number) {
  int i = 0, n = 0;
  bool stay = true;
  char cd[100];
  char name[100];
  struct st_pkt pkt;
  fd_set fds;
  struct timeval tv;
  // Set up the file descriptor set.
  FD_ZERO(&fds);
  FD_SET(sockfd, &fds);
  tv.tv_usec = 0; // ms waiting
  tv.tv_sec = 30; // s waiting
  n = select(sizeof(fds) * 8, &fds, NULL, NULL, &tv);
  if (n == 0) {
    printf("Server : Client non risponde\n");
    fflush(stdout);
    // specifico al client che ho chiuso la connessione
    pkt.finbit = 3;
    strcpy(pkt.pl, "Close");
    printf("Send finbit %d PL %s\n", pkt.finbit, pkt.pl);
    if ((sendto(sockfd, &pkt, sizeof(pkt), 0, (struct sockaddr *)&addr,
                addrlen)) < 0) {
      perror("errore in sendto");
      exit(1);
    }
  } else if (n == -1) {
    perror("Error send_control select");
    exit(1);
  } else if (n != 0 && n != -1) {
    if ((recvfrom(sockfd, &pkt, sizeof(pkt), 0, (struct sockaddr *)&addr,
                  &addrlen)) < 0) {
      perror("errore in recvfrom");
      exit(-1);
    }
    printf("SEND_CONTROL ::\n");
    fflush(stdout);
    while (pkt.pl[i] != ' ') {
      i++;
    }
    strncpy(cd, pkt.pl, i);
    // +1 perchè non voglio lo spazio
    strcpy(name, pkt.pl + i + 1);

    printf(" Server : Msg ricevuto %s\n", pkt.pl);
    fflush(stdout);
    /* ---------------------- gestisco caso get *
     * ----------------------------------------------*/

    if (!strcmp("get", cd)) {
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
        char fullPath[500]; // Percorso completo del file
        snprintf(fullPath, sizeof(fullPath), "%s/%s", "Server_Files",
                 entry->d_name);
        if (stat(fullPath, &fileStat) == 0 && S_ISREG(fileStat.st_mode) &&
            !strcmp(entry->d_name, name)) {
          found = true;
        }
      }
      if (found) {
        pkt.ack = 0;
        pkt.finbit = 0;
        strcpy(pkt.pl, "File trovato.");
        printf("\nServer : %s\n", pkt.pl);
        if ((sendto(sockfd, &pkt, sizeof(pkt), 0, (struct sockaddr *)&addr,
                    addrlen)) < 0) {
          perror("errore in sendto");
          exit(1);
        }
        // entro nella funzione che implementa la send del file
        send_get(name, sockfd);
      } else if (!found) {
        // Gestisco caso file non trovato
        strcpy(pkt.pl, "File non trovato, riprova.");
        pkt.ack = -1;
        pkt.finbit = 0;
        printf("invio ack con errore[%s]", pkt.pl);
        fflush(stdout);
        if ((sendto(sockfd, &pkt, sizeof(pkt), 0, (struct sockaddr *)&addr,
                    addrlen)) < 0) {
          perror("errore in sendto");
          exit(1);
        }
      }
      // Chiudi la cartella
      closedir(dir);
      // continue;
    }

    /* ---------------------------------- gestisco caso *
     * list------------------------------------------------*/

    if (!strcmp("list", cd)) {
      strcpy(pkt.pl, "List in esecuzione \n");
      pkt.ack = 0;
      pkt.finbit = 0;
      if ((sendto(sockfd, &pkt, sizeof(pkt), 0, (struct sockaddr *)&addr,
                  addrlen)) < 0) {
        perror("errore in sendto");
        exit(1);
      }
      send_list(sockfd);
    }

    /* ------------------------ gestisco caso put *
     * --------------------------------------*/
    if (!strcmp("put", cd)) {
      printf("\nStart command put\n");
      strcpy(pkt.pl, "Put  in esecuzione");
      pkt.ack = 0;
      pkt.finbit = 0;
      if ((sendto(sockfd, &pkt, sizeof(pkt), 0, (struct sockaddr *)&addr,
                  addrlen)) < 0) {
        perror("errore in sendto");
        exit(1);
      }
      rcv_put(name, sockfd);
    }
  }
  stop[my_number - 1] = false;
  printf("\nWait for next request my_number ind %d my bool value %d \n",
         my_number - 1, stop[my_number - 1]);
  fflush(stdout);
}

void child_main(int k) {
  int listenfd;
  int n_port = SERV_PORT + k;
  printf("child %ld starting my port number is %d\n", (long)getpid(), n_port);
  if ((listenfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) { /* crea il socket */
    perror("errore in socket");
    exit(1);
  }
  if ((listenfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) { /* crea il socket */
    perror("errore in socket");
    exit(1);
  }
  fd = listenfd;
  memset((void *)&addr, 0, sizeof(addr));
  addr.sin_family = AF_INET;
  addr.sin_addr.s_addr = htonl(INADDR_ANY); /* il server accetta pacchetti su
                            una qualunque delle sue interfacce di rete */
  addr.sin_port = htons(n_port); /* numero di porta data dal server la uso per
  accettare una richiesta

  /* assegna l'indirizzo al socket */
  if (bind(listenfd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
    perror("errore in bind");
    exit(1);
  }
  for (;;) {
    if (stop[k - 1]) {
      send_control(listenfd, k); /* processa la richiesta */
    }
  }
}
// creo i processi figli
pid_t child_make(int i) {
  pid_t pid;
  if ((pid = fork()) > 0)
    return (pid); // processo padre
                  //
  child_main(i);  // non ritorna mai
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
// termina i figli non ho stato di zombie
void sig_int(int signo) {
  int i;
  /* terminate all children */
  for (i = 0; i < nchildren; i++) {
    printf("\nkill child %d\n", pids[i]);
    kill(pids[i], SIGTERM);
  }
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
    fprintf(stderr, " utilizzo:<pkt loss probab P , SERV_Port, Inserire numero "
                    "dei child ,timeout> \n");
    exit(1);
  }
  // variabili richieste dalla traccia
  p = strtod(argv[1], NULL);
  SERV_PORT = atoi(argv[2]);
  nchildren = atoi(argv[3]);
  timeout = atoi(argv[4]);
  printf("\nProb inserita %f\n", p);
  printf("\nSERV_PORT inserito %d\n", SERV_PORT);
  printf("\nNumero childs %d\n ", nchildren);
  printf("\ntimeout in usec %d\n", timeout);
  stop = (bool *)mmap(NULL, sizeof(_Bool) * 25, PROT_READ | PROT_WRITE,
                      MAP_SHARED | MAP_ANONYMOUS, -1, 0);
  // creo un array di booleani per tenere traccia se il processo figlio è in
  // ascolto nella socket o è occupato
  int u = 0, n = 0;
  bool e = true;
  while (nchildren < 1 || nchildren > 25) {
    u++;
    char temp[100];
    printf("Numero thread deve essere >= 1 o <=25");
    fscanf(stdin, "%s", temp);
    nchildren = atoi(temp);
    if (u > 5) {
      exit(1);
    }
  }

  u = 0;
  if (SERV_PORT < 255) {
    char temp[100];
    fprintf(stderr, "inserire num. porta dedicato inserire un numero > 255");
    fscanf(stdin, "%s", temp);
    SERV_PORT = atoi(temp);
    if (u > 5) {
      exit(1);
    }
  }
  // variabili
  int listenfd;
  struct st_pkt pkt;
  void sig_int(int);

  if ((listenfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) { /* crea il socket */
    perror("errore in socket");
    exit(1);
  }
  memset((void *)&addrmain, 0, sizeof(addrmain));
  addrmain.sin_family = AF_INET;
  addrmain.sin_addr.s_addr =
      htonl(INADDR_ANY); /* il server accetta pacchetti su una qualunque delle
                            sue interfacce di rete */
  addrmain.sin_port = htons(SERV_PORT); /* numero di porta del server la uso per
  accettare una richiesta

  /* assegna l'indirizzo al socket */
  if (bind(listenfd, (struct sockaddr *)&addrmain, sizeof(addrmain)) < 0) {
    perror("errore in bind");
    exit(1);
  }
  ////////////////////////////////
  /////////////////////////////////
  // implemento la logica della prefork con file locking
  // qui alloco un area di memoria per mantenere i pid dei child
  pids = (pid_t *)calloc(nchildren, sizeof(pid_t));
  if (pids == NULL) {
    fprintf(stderr, "errore in calloc");
    exit(1);
  }

  for (int i = 0; i < nchildren; i++) {
    // inserisco nell array pids i pid dei figli che mi ritornano dalla
    // child_make
    // numero di porta che assegno ai child
    stop[i] = false;
    pids[i] = child_make(i + 1); /* parent returns */
  }
  if (signal(SIGINT, sig_int) == SIG_ERR) {
    fprintf(stderr, "errore in signal INT ");
    exit(1);
  }
  system("mkdir Server_Files");
  while (1) {
    fflush(stdout);
    if ((recvfrom(listenfd, &pkt, sizeof(pkt), 0, (struct sockaddr *)&addrmain,
                  &addrlen)) < 0) {
      perror("errore in recvfrom");
      exit(1);
    }
    printf("main rcvd pkt finbit %d \n", pkt.finbit);
    fflush(stdout);

    if (pkt.finbit == 2) {
      e = true; // serve per associare un solo child ad un solo client
      n = 0;
      for (int i = 0; i < nchildren; i++) {

        fflush(stdout);
        if (!stop[i] && e) {
          n = 1;
          e = false;
          pkt.ack = SERV_PORT + i + 1;
          sprintf(pkt.pl, "go");
          pkt.pl[2] = '\0';
          stop[i] = true;
          printf(" i = %d value bool = %d\n", i, stop[i]);
          printf("Server: invio pkt di go port number %d \n ", pkt.ack);
          if ((sendto(listenfd, &pkt, sizeof(pkt), 0,
                      (struct sockaddr *)&addrmain, addrlen)) < 0) {
            perror("errore in sendto");
            exit(1);
          }
        }
      }
    } else {
      printf("\nerrore di connessione\n");
      exit(1);
    }
    // se tutti i processi sono occupati invio un msg di wait al client
    // ritenterà in futuro
    if (n == 0) {
      printf("invio wait \n");
      fflush(stdout);
      sprintf(pkt.pl, "wait");
      pkt.pl[4] = '\0';
      if ((sendto(listenfd, &pkt, sizeof(pkt), 0, (struct sockaddr *)&addrmain,
                  addrlen)) < 0) {
        perror("errore in sendto");
        exit(1);
      }
    }
  }
}