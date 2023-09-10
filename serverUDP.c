#include <arpa/inet.h>
#include <asm-generic/errno-base.h>
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <locale.h>
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
//  variabili globali 
int seqnum = 0;
int timeout = 0;
int lt_ack_rcvd = 0;
int lt_rwnd = 1;
int swnd = 0;
int CongWin = 1;
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
socklen_t addrlen = sizeof(struct sockaddr_in);
bool rit = false;
bool s = true;
int dynamics_timeout=0;
typedef void Sigfunc(int);
bool adpt_timeout;
bool id_dup=false;
// array per mantenere i pkt
struct st_pkt *retr;
// pkt struct
struct st_pkt {
  int id;
  int code;
  char pl[MAXLINE];
  int rwnd;
};
//predichiaro le funzioni 
void *mretr();
void *rcv_cong(void *);
void send_get(char *, int );
void rcv_put(char *, int );
void send_list(int );
void send_control(int , int );
void child_main(int );
pid_t child_make(int );
Sigfunc *signal(int , Sigfunc *);
void sig_int(int);
///// 
//mretr: implementa la ritrasmissione dei pkt che il server ha inviato al client
void *mretr() {
  s = true;
  int check=0;
  while (s) {
    check=seqnum;
    puts("timeout started\n");
     //implemento il timeout
    usleep(dynamics_timeout);
    if (lt_ack_rcvd != check) {
      puts("timeout finished\n");
      rit = true;
      struct st_pkt pkt;
      int k;
       // dimezzo la ConWin
      if (CongWin > 1) {
        CongWin = CongWin>>1;
      }
      k = lt_ack_rcvd;
      //  implemento la ritrasmissione di tutti i pkt dopo lt_ack_rcvd
      swnd = 0;
      // ritrasmetto tutti i pkt dal pkt con id=last_ack_rcvd  fino all'ultimo pkt trasmesso
      while (k < seqnum) {
        while (swnd < CongWin && swnd < lt_rwnd && k < seqnum) {
           printf("MRETR:: ACK = %d  swnd = %d CongWin = %d  lt_rwnd = %d check = %d\n",
                 retr[k].id, swnd, CongWin, lt_rwnd,check);
          fflush(stdout);
          usleep(200);
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
      printf("Server : tutti i pkt sono stati ritrasmessi\n");
      fflush(stdout);
      if (k == dim) {
        s = false; 
      }
      rit = false;
    }
  }
}
//rcv_cong : processo che riceve i pkt e implementa i controlli 
void *rcv_cong(void *sd) {
  int sockfd;
  sockfd= *(int*)sd;
  fd = *(int*)sd;
  struct st_pkt pkt;
  int t = 0;
  pthread_t thread_id;
  lt_ack_rcvd = 0;
  //creo il thread per la ritrasmissione 
  bool stay = true;
  if (pthread_create(&thread_id, NULL, mretr, NULL) != 0) {
    perror("error pthread_create");
    exit(1);
  }  

  while (stay) {
    //aspetto di ricevere un pkt 
    if (recvfrom(sockfd, &pkt, sizeof(pkt), 0, (struct sockaddr *)&addr,
                 &addrlen) < 0) {
      perror("errore in recvfrom");
      exit(1);
    }
    //swnd: ci permette di tenere traccia del numero di pkt in "volo" decrementiamo tale numero ogni pkt ricevuto indifferentemente se l'id sia duplicato o nuovo
    if(swnd > 0)
    swnd--;
    printf("Ricevuto pkt: ack %d lt_rcvd %d swnd:%d\n", pkt.id, lt_ack_rcvd,swnd);
    fflush(stdout);
    //entro nell'if solo se il pkt ricevuto ha un id nuovo quindi non è arrivato fuori ordine
    if (pkt.id > lt_ack_rcvd ) {
      id_dup=false;
      //se adpt_timeout=true sole se l'utente ha inserito l'opzione di timeout dynamic
      if(adpt_timeout){
        dynamics_timeout=dynamics_timeout+500;  //timeout dinamico
      }
      CongWin++;
      lt_ack_rcvd = pkt.id;
      lt_rwnd = pkt.rwnd;//assegno un nuovo valore alla lt_rwnd che tiene conto della dimensione libera del buffer del client 
      // se è un id nuovo entro qui
      // se ricevo un pkt con payload = slow allora rallento in quanto il client sta scrivendo sul file
      if (!strcmp(pkt.pl, "slow")) {
        printf("\nSLOW RCVD\n");
        fflush(stdout);
        bytes_psecond = 100;
        //gestisco quanti bytes al secondo il processo può inviare trasmite la socket
        if (setsockopt(sockfd, SOL_SOCKET, SO_MAX_PACING_RATE, &bytes_psecond,
                       sizeof(bytes_psecond)) < 0) {
          perror("Error setsockopt");
          exit(1);
        }
      } else {
        bytes_psecond = 1000;
        //gestisco quanti bytes al secondo il processo può inviare trasmite la socket
        if (setsockopt(sockfd, SOL_SOCKET, SO_MAX_PACING_RATE, &bytes_psecond,
                       sizeof(bytes_psecond)) < 0) {
          perror("Error setsockopt");
          exit(1);
        }
      }
       //se pkt.code == 1 e pkt.id == seqnum allora chiudo la connessione ho ricevuto il terminatore
      if (pkt.code == 1 && pkt.id == seqnum) {
        printf("Server : Client disconesso  correttamente \n");
        fflush(stdout);
        stay = false;
        s = false; //blocco il while della ritrasmissione
        return NULL;  
      }
    } else { 
       // se ricevo un id duplicato allora imposto id_dup = true cosi da bloccare la trasmissione dei in questo caso della snd_put in quanto mi rendo conto che tutti i nuovi pkt inviati andranno comunque persi poichè arriveranno fuori ordine.
      //if(pkt.id != lt_ack_rcvd ) 
      id_dup=true;
      if(dynamics_timeout/2>timeout){
      dynamics_timeout>>1;
      }else{
        dynamics_timeout = timeout;
      } 
      } 
    }
  s = false;
   // aspetto la terminazione del thread che ritrasmette
  if (pthread_join(thread_id, NULL) != 0) {
    perror("Error pthread_join");
    exit(1);
  }
}
// gestice nello specifico il comando get
void send_get(char *str, int sockfd) {

  /* VALUTAZIONE PRESTAZIONI*/
  struct timeval begin, end;
  gettimeofday(&begin, 0);

   printf("\nServer : get alive \n");
   fflush(stdout);
  // variabili da resettare
  seqnum = 0;
  lt_ack_rcvd = 0;
  swnd = 0;
  CongWin = 1;
  int size = 0;
  lt_rwnd = 1;
  struct st_pkt pkt;
  pkt.code=0;
  pkt.id=0;
  pkt.rwnd=0;
  pkt.pl[0]='\0';
  FILE *file;
  bool stay = true;
  int i = 0, msgInviati = 0, msgPerso = 0, msgTot = 0, dimpl = 0;
  double prob = 0;
  char path_file[MAXLINE];
  path_file[0]='\0';
  pthread_t thread_id;
  ///
    //apro il file da leggere
  sprintf(path_file, "Server_Files/%s", str);
  if ((file = fopen(path_file, "r+")) == NULL) {
    printf("Errore in open del file\n");
    exit(1);
  }
  // ottengo la dimesione del file cosi da definire la dimensione dell'array che conterra i pkt che saranno poi ritrasmessi in caso di fine timeout
  fseek(file, 0, SEEK_END);     // seek to end of file
  size = ftell(file);           // get current file pointer
  fseek(file, 0, SEEK_SET);     // seek back to beginning of file
  dim = ((size) / MAXLINE) + 1; // +1 perchè arrotonda per difetto
  printf("dim : %d\n",dim);
  //sleep(1);
  //alloco l'araay che mantiene i pkt per la ritrasmissione
  if ((retr = malloc(sizeof(struct st_pkt) * dim)) == NULL) {
    perror("Error malloc");
    exit(1);
  }
  // creo il thread che mi legge le socket (lettura bloccante)
  if (pthread_create(&thread_id, NULL, rcv_cong, &sockfd) != 0) {
    perror("error pthread_create");
    exit(1);
  }
  while (stay) {
     // se il numero di pkt in volo è < della dimensione della CongWin e < dello spazio ancora libero in memoria lato server ed non sto ritrasmettendo e non ho ricevuto un id duplicato entro 
    while (swnd < CongWin && stay == true && swnd < lt_rwnd && !rit && !id_dup) {
      //leggo il file finchè la fread non legge meno di MAXLINE in questo caso sono arrivato a fine file
      if ((dimpl = fread(pkt.pl, 1, sizeof(pkt.pl), file)) == MAXLINE) {
        seqnum++;//aumento il numero di pkt 
        swnd++;//aumento il numero di pkt in "volo"
        pkt.code = 0;//imposto il pkt come informativo 
        pkt.id = seqnum;
        //mantengo il pkt nell'array
        retr[i] = pkt;
        i++;
        // aumento la dim del vettore che mi salva i pkt
        prob = (double)rand() / RAND_MAX;//implemento la probabilità di errore
        if (prob < p) {//implemento l'errore
          msgPerso++;
          msgTot++;
          // Il messaggio è stato perso
          printf(" LOST :: ACK = %d  swnd = %d CongWin = %d  lt_rwnd = %d\n",
                 pkt.id, swnd, CongWin, lt_rwnd);
        } else {
          // Trasmetto con successo
         /* if (lt_ack_rcvd < seqnum - 10) { // se lt_ack è molto piccolo rallento(flow_control)
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
        }*/
          printf("SEND_GET :: ACK = %d  swnd = %d CongWin = %d  lt_rwnd = %d\n",
                 pkt.id, swnd, CongWin, lt_rwnd);
          fflush(stdout);
         
            usleep(200);
          if ((sendto(sockfd, &pkt, sizeof(pkt), 0, (struct sockaddr *)&addr,
                      addrlen)) < 0) {
            perror("errore in sendto");
            exit(1);
          }
          msgInviati++;
          msgTot++;
        }
      } else if (feof(file)) {//entro solo se ho letto tutto il file
        while (lt_ack_rcvd != seqnum) { //aspetto di sincronizzare last ack rcvd con il numero associato all'ultimo pkt trasmesso
            usleep(200);//sleep cosi da non rimanere schedulato
        }
        seqnum++; //aumento il seqnum 
        swnd++;//incremento il numero di pkt in volo
        pkt.id = seqnum;
        pkt.code = 1; //pkt.cod=1 pkt di chiusura ho letto tutto il file 
        pkt.pl[dimpl] = '\0';// payload vuoto 
        retr[i] = pkt;//mantengo il pkt per la ritrasmissione
        stay = false;
        printf("\nServer : send last pkt %d  \n", seqnum);
        prob = (double)rand() / RAND_MAX;//implemento la probabilità di errore
        if (prob < p){
          // Il messaggio è stato perso
          msgPerso++;
          msgTot++;
          continue;
        } else {
          printf("\n Server : Close connection \n");
          fflush(stdout);
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
  // aspetto la terminazione del thread che legge(rcv_cong)
  if (pthread_join(thread_id, NULL) != 0) {
    perror("Error pthread_join");
    exit(1);
  }

  /*VALUTAZIONE PRESTAZIONI*/
  gettimeofday(&end, 0);
  long seconds = end.tv_sec - begin.tv_sec;
  long microseconds = end.tv_usec - begin.tv_usec;
  double elapsed = seconds + microseconds*1e-6;
  printf("Get ha impiegato: %.4f seconds.\n", elapsed);
  printf("\nMSG TOTALI %d\n", msgTot);
  printf("\nMSG PERSI %d\n", msgPerso);
  printf("\nMSG INVIATI CORRETAMENTE %d\n", msgInviati);
  printf("\nMSG RITRASMESSI %d\n",msgRitr);
  printf("\n Dim CongWin finale %d\n", CongWin);
  printf("\n Swnd finale : %d\n", swnd);
  printf("\n PROB DI SCARTARE UN MSG %f\n", p);
  //printf("\n TIMEOUT FINALE  %d\n", dynamics_timeout);
  fflush(stdout);
   //libero-chiudo i vari descrittori
  free(retr);
  fclose(file);
}
// gestice nello specifico il comando put
void rcv_put(char *file, int sockfd){
  printf("\n Server : Put function alive\n");
  fflush(stdout);
  dim = 4096; //fissiamo il valore dell'array che mantiene i pkt per poi scriverli sul file
  free_dim = dim;
  struct st_pkt pkt;
    //alloco l'array che mantiene i pkt
  if ((retr = malloc(sizeof(struct st_pkt) * dim)) == NULL) {
    perror("Error malloc");
    exit(1);
  }
  FILE *fptr;
  // creo il file se già esiste lo cancello tanto voglio quello aggiornato
  int n = 0, i = 0;
  bool stay = true, different = true;
  char path_file[MAXLINE];
  sprintf(path_file, "Server_Files/%s", file);
  if ((fptr = fopen(path_file, "w+")) == NULL) {
    perror("Error opening file");
    exit(1);
  }
  while (stay) {
    // incremento n se ho ricevuto un pkt con un id nuovo 
    if (different) {
      n++;
    }
    //aspetto di ricevere i pkt dal server
    if ((recvfrom(sockfd, &pkt, sizeof(pkt), 0, (struct sockaddr *)&addr,&addrlen)) < 0) {
      perror("errore in recvfrom");
      exit(1);
    }
   // se pkt.code == 1  e pkt.id == n allora chiudo la connessione(pkt di chiusura e con id che mi aspettavo) 
    if (pkt.code == 1 && pkt.id == n){
      printf("\nServer  : last pkt :  ack = %d free_dim %d\n", pkt.id, free_dim);
      fflush(stdout);
      printf("\n Server : Client close connection \n");
      retr[i] = pkt;
      i++;
      // cicliclo
      i = i % dim;
      free_dim--;
      pkt.id = n;
      pkt.code = 1;//invio un pkt di chiusura confermo la chiusura al server
      pkt.rwnd = free_dim;
      printf("\n Server  : Confermo chiusura\n");
      //invio il pkt
      if (sendto(sockfd, &pkt, sizeof(pkt), 0, (struct sockaddr *)&addr,
                 addrlen) < 0) {
        perror("errore in sendto");
        exit(1);
      }
      // se chiudo e la free_dim è != 0 allora devono scrivere sul file dato che non scrivo subito il payload di un pkt che ho ricevuto ma solo se ho occupato tutto l'array che mantiene i vari pkt(più performante)
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
      // se pkt.code = 0 e pkt.id = n allora ho ricevuto un pkt informativo con un id che mi aspettavo lo immagazzino e se free_dim = 0 scrivo tutti i pkt salvati sul file 
    } else if (pkt.code == 0 && pkt.id == n) {
      retr[i] = pkt;
      i++;
      // cicliclo
      i = i % dim;
      different = true;
      free_dim--;
      printf("Server : pkt ricevuto : ack = %d free_dim %d\n", pkt.id, free_dim);
      fflush(stdout);
      pkt.id = n;
      pkt.code = 0;
      pkt.rwnd = free_dim;
      //se free_dim = 0 per prima cosa invio subito la slow per rallentare il server 
      if (free_dim == 0) {
        printf("Send slow to Client\n");
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
      //scrivo i dati ricevuto sul file 
      if (free_dim == 0) {
        int t = 0;
        free_dim = dim;
        for (t = 0; t < dim; t++) {
          if ((fwrite(retr[t].pl, strlen(retr[t].pl), 1, fptr) < 0)) {
            perror("Error in write rcv_get\n");
            exit(1);
          }
        }
        printf("Server: write all pkt and send msg to Server\n");
        fflush(stdout);
      }
    }
    // se arriva un pkt fuori ordine invio subito id non faccio la
    // bufferizzazione lato rcv
    else if (stay == true && pkt.id != n) {
      // non incremento n pongo diff = true
      different = false;
      // invio al sender un id comulativo fino a dove ho ricevuto
      printf("Server : pkt fuori ordine num %d  invio id %d \n", pkt.id, n - 1);
      fflush(stdout);
      pkt.id = n - 1;
      pkt.rwnd = free_dim;
      if (sendto(sockfd, &pkt, sizeof(pkt), 0, (struct sockaddr *)&addr,
                 addrlen) < 0) {
        perror("errore in sendto");
        exit(1);
      }
    }
  }
  //chiudo-free i vari puntatori
 fclose(fptr);
 free(retr);
}
// gestice nello specifico il comando list
void send_list(int sockfd) {
  /* VALUTAZIONE PRESTAZIONI*/
  struct timeval begin, end;
  gettimeofday(&begin, 0);
  printf("Server : Send_list Alive\n");
  //variabili resettate 
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
  int i = 0, msgInviati = 0, msgPerso = 0, msgTot = 0;
  double prob = 0;
  pthread_t thread_id;
  char **nomi = malloc(sizeof(char *) * 256); //array che mantiene i nomi dei file 
  int c = 1;//variabili usata per mantenere il numero di nomi letti
  //array per mantenere i pkt per la ritrasmissione
  if ((retr = malloc(sizeof(struct st_pkt) * dim)) == NULL) {
    perror("Error malloc");
    exit(1);
  }
// creo il thread che implementa la ricezione e il loro controllo
  if (pthread_create(&thread_id, NULL, rcv_cong,&sockfd) != 0) {
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
  usleep(50);
  //leggo i nomi dei file nella directory e li salvo 
  while ((file = readdir(directory)) != NULL) {
    if (strcmp(file->d_name, ".") != 0 && strcmp(file->d_name, "..") != 0) {
      if ((nomi[c - 1] = malloc(strlen(file->d_name) + 1)) == NULL) {
        perror("error malloc ");
        exit(1);
      }
      strncpy(nomi[c - 1], file->d_name, strlen(file->d_name) + 1);
        printf("%s\n", nomi[c-1]);

      c++;
    }
  }
  closedir(directory);
  if (!file) {
    c--;
  }
  //se c==0 non ho letto nessun nome allora invio tale risultato al client
  if (c == 0) {

    puts("nessun file nella dir\n");

    pkt.id = -1;

    strcpy(pkt.pl, "nessun file in dir");

    if ((sendto(sockfd, &pkt, sizeof(pkt), 0, (struct sockaddr *)&addr,
                addrlen)) < 0) {
      perror("errore in sendto");
      exit(1);
      free(nomi);
    }
    return;
  }
  int temp = 0;

  while (stay) {
    // se il numero di pkt in volo è < della dimensione della CongWin e < dello spazio ancora libero in memoria lato server ed non sto ritrasmettendo e non ho ricevuto un id duplicato entro 
    while (swnd < CongWin && stay == true && swnd < lt_rwnd && !rit && !id_dup) {
      //se ho letto almeno un nome entro
      if(c>0){
      temp = strlen(nomi[c - 1]);
      strncpy(pkt.pl, nomi[c - 1], temp);
      free(nomi[c-1]);
      printf("pkt.pl %s \n", pkt.pl);
      fflush(stdout);
      pkt.pl[temp] = '\0'; // risolvo il problema di scrivere sul buff pieno
      swnd++;//aumento il numero di pkt in "volo"
      seqnum++;//aumento il numero di pkt 
      c--; // decremento
      pkt.id = seqnum;
      pkt.code = 0;//imposto il pkt come informativo 
      retr[i] = pkt;//mantengo il pkt nell'array
      i++;
      prob = (double)rand() / RAND_MAX;//mantengo il pkt nell'array
      if (prob < p) {//implemento l'errore
        msgPerso++;
        msgTot++;
        // Il messaggio è stato perso
        continue;
      } else {
        if ((sendto(sockfd, &pkt, sizeof(pkt), 0, (struct sockaddr *)&addr,
                    addrlen)) < 0) {
          perror("errore in sendto");
          exit(1);
        }

        printf("Server : list :: ack = %d  swnd = %d CongWin = %d  lt_rwnd = %d\n",pkt.id, swnd, CongWin, lt_rwnd);
        fflush(stdout);
        msgInviati++;
        msgTot++;
      }
      }
      // mando l'ultimo pkt
      else if (c == 0) {
        swnd++;//incremento il numero di pkt in volo
        // Invio l'ultimo pkt chiudo la connessione
        stay = false;
        pkt.code = 1;//pkt.cod=1 pkt di chiusura ho letto tutto il file 
        seqnum++;//aumento il seqnum 
        pkt.id = seqnum;
        pkt.pl[0] = '\0';// payload vuoto 
        retr[i] = pkt;//mantengo il pkt per la ritrasmissione
        prob = (double)rand() / RAND_MAX;//implemento la probabilità di errore
        printf("Server : Invio ultimo pacchetto. \n");
        if (prob < p) {
          // Il messaggio è stato perso
          msgPerso++;
          msgTot++;
        } else {
          // invio il terminatore
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
// aspetto la terminazione del thread che legge(rcv_cong)
  if (pthread_join(thread_id, NULL) != 0) {
    perror("Error pthread_join");
    exit(1);
  }
  printf("Server : Connessione terminata corretamente\n");
  
  // Chiusura della cartella  
  printf("Server : Chiudo la directory\n");
    /*VALUTAZIONE PRESTAZIONI*/
  gettimeofday(&end, 0);
  long seconds = end.tv_sec - begin.tv_sec;
  long microseconds = end.tv_usec - begin.tv_usec;
  double elapsed = seconds + microseconds*1e-6;
  printf("Get ha impiegato: %.4f seconds.\n", elapsed);
  printf("\nMSG TOTALI %d\n", msgTot);
  printf("\nMSG PERSI %d\n", msgPerso);
  printf("\nMSG INVIATI CORRETAMENTE %d\n", msgInviati);
  printf("\nMSG RITRASMESSI %d\n",msgRitr);
  printf("\n Dim CongWin finale %d\n", CongWin);
  printf("\n Swnd finale : %d\n", swnd);
  printf("\n PROB DI SCARTARE UN MSG %f\n", p);
  printf("\n TIMEOUT FINALE  %d\n", dynamics_timeout);
  //libero-chiudo i vari descrittori
  free(retr);  
  free(nomi);
  return;
}


/*Gestitsco la richiesta a livello applicativo inviata dal client. */
void send_control(int sockfd, int my_number) {

printf("Processo child numero: %ld \n", (long)getpid());
fflush(stdout);
  int i = 0, n = 0; /* Variabili contatori. */
  bool stay = true; /*Booleano per indicare se rimanere nel while principale o uscire. */
  char command_received[100]; /*Buffer per inserire il comando ricevuto dal payload. */
  char file_name[100]; /*Buffer per inserire il nome del file da inviare in caso di GET. */

  struct st_pkt pkt; /*Dichiaro un pacchetto. */


/*Creo una select che aspetterà 60 secondi. Se in questi 60 secondi non viene scritto nulla sulla socket
allora il client non ha risposto in tempo, e verrà inviato un pacchetto di CLOSE con code = 3, e verrà chiusa la connessione. */
  fd_set fds;
  struct timeval tv;
  FD_ZERO(&fds);
  FD_SET(sockfd, &fds);
  tv.tv_usec = 0;
  tv.tv_sec = 40;
  n = select(sizeof(fds) * 8, &fds, NULL, NULL, &tv);
  if (n == 0) {
    printf("Server : Client non risponde\n");
    fflush(stdout);
    pkt.code = 3;
    strcpy(pkt.pl, "Close");
    printf("Server : invio code %d. PL: %s \n", pkt.code, pkt.pl);
    if ((sendto(sockfd, &pkt, sizeof(pkt), 0, (struct sockaddr *)&addr,
                addrlen)) < 0) {
      perror("errore in sendto");
      exit(1);
    }
  } 
  
/*Controllo in caso di errore della select. */
  else if (n == -1) {
    perror("Error send_control select");
    exit(1);
  } 
  
  
/*In ogni altro caso ricevo un pacchetto. Leggo il payload fino al primo spazio che per convenzione indica dove finisce il comando. 
Sempre per costruzione del pacchetto, dopo il comando in caso di GET sarà scritto file_name da inviare. Quindi salvo sia il comando che
il nome del file (se presente), e gestisco i vari casi. */
  else if (n != 0 && n != -1) {
    if ((recvfrom(sockfd, &pkt, sizeof(pkt), 0, (struct sockaddr *)&addr,
                  &addrlen)) < 0) {
      perror("Errore in recvfrom");
      exit(-1);
    }
    while (pkt.pl[i] != ' ') {
      i++;
    }
    strncpy(command_received, pkt.pl, i);
    command_received[i]='\0';
    if(strcmp("list", command_received)) {
      strcpy(file_name, pkt.pl + i + 1);
    file_name[strlen(pkt.pl +i + 1)]='\0';
    }

/*CASO QUIT : Client scollegato chiudo la connesione */
if ( pkt.code == 3 && !strcmp("quit",command_received)) {
  printf("Server : Client close connection\n");
}

/* CASO GET : Se ricevo un comando di get, dichiaro una variabile found, che mi servirà per tenere traccia se il file è stato trovato o 
    no. Una variabile dir e altre strutture dati che servono per navigare nel file system e cercare il file. */
    if (!strcmp("get", command_received)) {
      bool found = false;
      DIR *dir;
      struct dirent *entry;
      struct stat fileStat;


/*Apro la cartella con opendir, successivamente controllo se è avvenuto con successo. */
      dir = opendir("Server_Files");
      if (dir == NULL) {
        perror("Impossibile aprire la cartella");
        exit(1);
      }    

/*Leggo nella cartella ogni entry, e scrivo nella variabile fullPath il percorso del file appena letto e faccio un controllo prima di 
comparare il file letto con file_name. Se risultano uguali allora found è TRUE, altrimenti rimarrà FALSE. */
      while ((entry = readdir(dir)) != NULL && !found) {
        char fullPath[500]; // Percorso completo del file
        snprintf(fullPath, sizeof(fullPath), "%s/%s", "Server_Files",
                 entry->d_name);
        if (stat(fullPath, &fileStat) == 0 && S_ISREG(fileStat.st_mode) &&
            !strcmp(entry->d_name, file_name)) {
          found = true;
        }
      }
/*Se ho trovato il nome del file, invio un pacchetto con id = 0, e code = 0, per indicare che
si tratta di un pacchetto informativo(code = 0) ed è il primo. (id)*/
      if (found) {
        closedir(dir);
        pkt.id = 0;
        pkt.code = 0;
        strcpy(pkt.pl, "File trovato.");
        printf("\nServer : %s\n", pkt.pl);
        if ((sendto(sockfd, &pkt, sizeof(pkt), 0, (struct sockaddr *)&addr,addrlen)) < 0) {
          perror("Errore in sendto");
          exit(1);
        }
/*La funzione send_get è la funziona che si occupa di inviare il file,  e come parametri avrà file_name e la socket. */
        send_get(file_name, sockfd);
      }      
/*Nel caso in cui il file non venga trovato, invio un paccheto con id = -1, ovvero indica un errore ed esco.*/
      else if (!found) {
        closedir(dir);
        strcpy(pkt.pl, "File non trovato, riprova.");
        pkt.id = -1;
        pkt.code = 0;
        printf("Server: invio errore: [%s]", pkt.pl);
        fflush(stdout);
        if ((sendto(sockfd, &pkt, sizeof(pkt), 0, (struct sockaddr *)&addr,
                    addrlen)) < 0) {
          perror("Errore in sendto");
          exit(1);
        }
      }
    }
/*CASO LIST : Se ricevo un comando LIST provvedo a inviare un pacchetto con id = 0, e code = 0 per indicare un ACK. */
    if (!strcmp("list", command_received)) {
      strcpy(pkt.pl, "List in esecuzione \n");
      pkt.id = 0;
      pkt.code = 0;
      if ((sendto(sockfd, &pkt, sizeof(pkt), 0, (struct sockaddr *)&addr,
                  addrlen)) < 0) {
        perror("errore in sendto");
        exit(1);
      }
/*Avvio la funzione send_list che si occuperà di inviare la lista degli elementi presenti nella cartella server_files. */           
      send_list(sockfd);
    }
/*CASO PUT : Gestisco il caso in cui ricevo un comando PUT. Invio un ack come nel caso LIST. */
    if (!strcmp("put", command_received)) {
      strcpy(pkt.pl, "Put  in esecuzione");
      pkt.id = 0;
      pkt.code = 0;
      if ((sendto(sockfd, &pkt, sizeof(pkt), 0, (struct sockaddr *)&addr,
                  addrlen)) < 0) {
        perror("errore in sendto");
        exit(1);
      }
//Chiamo la funzione rcv_put che si occupa di gestire la ricezione dei pacchetti inviati dal client.
      rcv_put(file_name, sockfd);
    }
  }
/*Una volta terminata la send_control, imposto il vettore stop = false, in corrispondenza del numero del processo. E torno nella child_main*/
  stop[my_number - 1] = false;
  fflush(stdout);
  printf("\n Child %ld wait for next request\n", (long)getpid() );
  fflush(stdout);
}
/*Funzione che verrà eseguita all'inizio da ogni child process.*/
void child_main(int k) {
/*Assegno ad ogni child process una socket con lo stesso procedimento fatto nel main. */
  int listenfd;
  int n_port = SERV_PORT + k;
  printf("Processo child numero: %ld. Numero di porta: %d\n", (long)getpid(), n_port);
  if ((listenfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) { 
    perror("Errore in socket");
    exit(1);
  }
  fd = listenfd;
  memset((void *)&addr, 0, sizeof(addr));
  addr.sin_family = AF_INET;
  addr.sin_addr.s_addr = htonl(INADDR_ANY); 
  addr.sin_port = htons(n_port); 
  if (bind(listenfd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
    perror("Errore in bind");
    exit(1);
  }
/*Se nel main viene accettata ed assegnata una richiesta al processo, il vettore STOP viene impostato a TRUE per l'indice 
interessato. Se il processo ha sul suo relativo indice TRUE, entrerà nella funzione send_control, che gestisce le richieste 
applicative del client. */
  while(1){ 
    if (stop[k - 1]) { 
      send_control(listenfd, k); /* processa la richiesta */
    }
  }
}
/* Funziona che crea i child_process. */
pid_t child_make(int i) {
  pid_t pid;
/*Ritorno al processo padre il PID. */
  if ((pid = fork()) > 0)
    return (pid); 
                  

/*Avvio la funzione del child. */
  child_main(i);
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


/* Tutti i commenti fanno riferimento al codice sottostante.*/
int main(int argc, char **argv) {
  if (argc != 6) {
    fprintf(stderr, "Utilizzo: ./serverUDP <probabilita' di errore> < numero di porta > < numero di processi child > < timeout in us > < timeout adattivo s = si n = no > \n");
    exit(1);
  }

  int u = 0,y=0;
  char ertt[100],buff[MAXLINE];
  /*Prendo in input da terminale le variabili necessarie */
  p = strtod(argv[1], NULL);
  SERV_PORT = atoi(argv[2]);
  nchildren = atoi(argv[3]);
  timeout = atoi(argv[4]);
  strcpy(ertt,argv[5]);
  if(!strcmp(ertt,"s")){
    adpt_timeout=true;
  }else if(!strcmp(ertt,"n")){
    adpt_timeout=false;
  }
  //controllo sul valore del timeout
  while(timeout < 500 || timeout > 120000000 ){
    u++;
    printf("inserire un timeout 0.5ms < timeout < 120s\n");
    if (y=(fscanf(stdin, "%d", &timeout)) == EOF) {
        perror("Error fscanf");
        exit(1);
      }
      if(y < 1){
        fgets(buff,MAXLINE,stdin);
        buff[0]='\0';
      }
    if (u > 5) {
      printf("Inserito troppe volte il numero sbagliato\n");
      exit(1);
    }
  }
  //controllo sulla probabilità di errore
  while( p > 1 || p < 0){
    u++;
    printf("inserire una probabilità di errore compresa tra 0 e 1\n");
    if (y=(fscanf(stdin, "%le", &p)) == EOF) {
        perror("Error fscanf");
        exit(1);
      }
      if(y < 1){
        fgets(buff,MAXLINE,stdin);
        buff[0]='\0';
      }
    if (u > 5) {
      printf("Inserito troppe volte il valore sbagliato\n");
      exit(1);
    }
  }
  //controllo sul timeout dinamico
  while(strcmp(ertt,"s") && strcmp(ertt,"n")){
    u++;
    printf("inserire s (se si vuole un timeout dinamico) o n (altrimenti)  \n");
    if(fgets(ertt,4,stdin) == NULL ){
      perror("errore fgets");
      exit(1);
    }
    ertt[1]='\0';
    printf("ertt == %s\n",ertt);
  if(!strcmp(ertt,"s")){
    adpt_timeout=true;
  }else if(!strcmp(ertt,"n")){
    adpt_timeout=false;
  }
  if (u > 5) {
      printf("Inserito troppe volte il valore sbagliato\n");
      exit(1);
    }
  }
  u=0;
    //controllo sul valore dei figli
  while (nchildren < 1 || nchildren > 25) {
    u++;
    printf("Il numero dei child deve essere >= 1 o <=25");
    if (y=(fscanf(stdin, "%d", &nchildren)) == EOF) {
        perror("Error fscanf");
        exit(1);
      }
      if(y < 1){
        fgets(buff,MAXLINE,stdin);
        buff[0]='\0';
      }
    if (u > 5) {
      printf("Inserito troppe volte il numero sbagliato\n");
      exit(1);
    }
  }
  u = 0;

  //controllo sul valore del Server port
  while (SERV_PORT < 255) {
    u++;
    char temp[100];
    temp[0]='\0';
    fprintf(stderr, "Numero porta riservato. Inserire un nuovo valore \n");
    if (y=(fscanf(stdin, "%d", &SERV_PORT)) == EOF) {
        perror("Error fscanf");
        exit(1);
      }
      if(y < 1){
        fgets(buff,MAXLINE,stdin);
        buff[0]='\0';
      }
    if (u > 5) {
      printf("Inserito troppe volte il numero sbagliato\n");
      exit(1);
    }
  }
  printf("\nProb inserita %f\n", p);
  printf("\nSERV_PORT inserito %d\n", SERV_PORT);
  printf("\nNumero childs %d\n ", nchildren);
  printf("\nTimeout in usec %d\n", timeout);
  printf("\n Timeout dinamico %d\n", adpt_timeout);
  dynamics_timeout=timeout;

/* Il vettore stop sottostante, è un vettore di booleani in memoria condivisa. Questo vettore tiene traccia di quali processi sono attivi
ed in ascolto su una determinata socket. E' in memoria condivisa cosicchè tutti i processi possano tenerne traccia.*/
  stop = (bool *)mmap(NULL, sizeof(_Bool) * 25, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);

  int n = 0,k=0,ind_child=0;
  bool e = true;

/* Dichiaro le variabili che serviranno ad inizializzare le socket. */
  int listenfd;
  struct st_pkt pkt;
  void sig_int(int);
  if ((listenfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) { /* Creo la socket */
    perror("Errore in socket");
    exit(1);
  }

  addr.sin_family = AF_INET;


/*Con questo parametro il server accetta pacchetti su una qualunque delle sue interfacce di rete */
  addr.sin_addr.s_addr = htonl(INADDR_ANY); 

                            
/* Assegno alla socket il numero di porta inserito da terminale */                          
  addr.sin_port = htons(SERV_PORT); 


/* Assegno l'indirizzo alla socket. */
  if (bind(listenfd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
    perror("Errore in bind");
    exit(1);
  }


/*Creo un array per salvare i pids dei processo child. */
  pids = (pid_t *)calloc(nchildren, sizeof(pid_t));
  if (pids == NULL) {
    fprintf(stderr, "Errore in calloc");
    exit(1);
  }


/*In questo ciclo for inserisco nell'array "pids" i PID dei figli che ottengo dalla funzione child_make*/
  for (ind_child = 0; ind_child < nchildren; ind_child++) {
    stop[ind_child] = false;
    pids[ind_child] = child_make(ind_child + 1); /* parent returns */
  }


  /*Indico la funzione che gestirà il segnale di INT.*/
  if (signal(SIGINT, sig_int) == SIG_ERR) {
    fprintf(stderr, "Errore in signal INT ");
    exit(1);
  }


  /*Creo la cartella Server_Files, se già esiste, il terminale riporterà un warning. */
  system("mkdir Server_Files");


/*In questo while, il processo padre è in ascolto sulla socket inserita da terminale, in attesa di richieste da parte del client.
Nel momento in cui avviene una richesta, ed il code è 2, cerco un processo non impegnato, gli assucio una nuova porta, invio il pacchetto
di GO al client, che successivamente invierà la richiesta di GET, LIST oppure di PUT.
Nel caso in cui tutti i processi siano impegnati, verrà inviato un pacchetto di WAIT, che farà aspettare il client che ritenterà successivamente
a farsi assegnare un processo.*/
  while (1) {
    fflush(stdout);
    if ((recvfrom(listenfd, &pkt, sizeof(pkt), 0, (struct sockaddr *)&addr,
                  &addrlen)) < 0) {
      perror("Errore in recvfrom");
      exit(1);
    }
    printf("Server: ricevuto pacchetto con code : [%d]\n", pkt.code);
    fflush(stdout);
    //se ricevo un pkt con code=2 allora è un pkt di richiesta di connesione
    if (pkt.code == 2) {
      e = true;
      n = 0;//variabile usata per determinare se ho assegnato una richiesta o meno
      for (int i = 0; i < nchildren; i++) { //scorro tutto l'array di bool per determinare se un processo è libero e può eseguire la richiesta del client
        fflush(stdout);
        if (!stop[i] && e) {  //se e=true e quindi ho ricevuto una richiesta da parte di un client e c'è un elemento dell'array stop a false allora vuol dire che viè almeno un processo libero che può quindi eseguire la richiesta del client
          printf("Child with number %d free assign request\n",i);
          fflush(stdout);
          n = 1;  //n=1 allora ho soddisfatto almeno una richiesta 
          e = false;
          pkt.id = SERV_PORT + i + 1; //invio al client la porta del processo che può eseguire la richiesta 
          sprintf(pkt.pl, "go");//invio un pkt con payload=go
          pkt.pl[2] = '\0';
          stop[i] = true; //pongo il bool a true dato che il processo è occupato dalla nuova richiesta
          k=0;
          printf(" i = %d value bool = %d\n", i, stop[i]);
          printf("Server: invio pkt di go port number %d \n ", pkt.id);
          if ((sendto(listenfd, &pkt, sizeof(pkt), 0, (struct sockaddr *)&addr,
                      addrlen)) < 0) {
            perror("errore in sendto");
            exit(1);
          }
        }
      }
    } else {
      printf("\nerrore di connessione\n");
      exit(1);
    }
    //se n==0 allora tutti i processi sono occupati
    if (n == 0) {
      k++;
      if(k > 2){
        nchildren++;
        pids = (pid_t *)reallocarray(pids,nchildren, sizeof(pid_t));
        if (pids == NULL){
            fprintf(stderr, "Errore in realloc");
            exit(1);
          }
          if(nchildren <=25){
            printf("Server : molte richieste di accesso creo un nuovo figlio \n");
            stop[ind_child] = false;
            pids[ind_child] = child_make(ind_child + 1); /* parent returns */
            ind_child++;
          }
      }
      printf("invio wait \n");
      fflush(stdout);
      sprintf(pkt.pl, "wait");
      pkt.pl[4] = '\0';
      if ((sendto(listenfd, &pkt, sizeof(pkt), 0, (struct sockaddr *)&addr,
                  addrlen)) < 0) {
        perror("errore in sendto");
        exit(1);
      }
    }
  }
}