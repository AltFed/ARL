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
#include <sys/time.h>
#include <unistd.h>
#include <wait.h>
#define MAXLINE 4096
typedef void Sigfunc(int);
//variabili globali 
int SERV_PORT;
bool loop = false;
int dim = 0;
int free_dim = 0;
int seqnum = 0;
int lt_ack_rcvd = 0;
int num = 0;
int lt_rwnd = 1;
int fd;
int swnd = 0;
int msgRitr = 0;
int CongWin = 1;
int timeout = 0;
double p = 0;
bool s = true;
long int bytes_psecond = 0;
bool rit = false;
int sockfd; // descrittore alla socket creata per comunicare con il server
struct sockaddr_in addr;
struct st_pkt *rcv_win;
int dynamics_timeout=0;
bool adpt_timeout;
bool id_dup=false;
socklen_t addrlen = sizeof(struct sockaddr_in);
// struttura del pkt
struct st_pkt {
  int id;
  int code;
  char pl[MAXLINE];
  int rwnd;
};
//predichiaro le funzioni 
int port_number(int);
void req();
void command_send(char *, char *);
void *mretr();
void *rcv_cong(void *);
void rcv_get(char *);
void snd_put(char *, int );
void rcv_list();
Sigfunc *signal(int , Sigfunc *);
void sig_int(int);
///////////////
//port_number : invia un pkt al server con pkt.code=2 (un pkt di richiesta) che permette al client di ottenere il numero di porta di un processo child del server disponibile a soddisfare la richiesta.
int port_number(int sockfd) { 
  addr.sin_port = htons(SERV_PORT);
  struct st_pkt pkt;
  int temp = 0,n=0;
  pkt.code = 2;
  pkt.pl[0] = '\0';
  pkt.id = 0;
  pkt.rwnd=0;
  fd_set fds;
  struct timeval tv;
  FD_ZERO(&fds);
  FD_SET(sockfd, &fds);
  tv.tv_usec = 0;
  tv.tv_sec = 5;
  bool rcv_winy = true;
  printf("Port_number : send request to Server\n");
  fflush(stdout);
  if (sendto(sockfd, &pkt, sizeof(pkt), 0, (struct sockaddr *)&addr, addrlen) < 0) {
    perror("errore in sendto");
    exit(1);
  }
  while(pkt.id < SERV_PORT){  //il child non può avere un numero di porta minore del server 
    n = select(sizeof(fds) * 8, &fds, NULL, NULL, &tv);
    if(n == 0 ){
      //se la select scade vuol dire che ho inviato un pkt al server ma nessuno ha risposto allora il processo del server che gestisce le richieste è off
      printf("Client: Server ancora non connesso aspetto\n");
      sleep(3);
      if (sendto(sockfd, &pkt, sizeof(pkt), 0, (struct sockaddr *)&addr, addrlen) < 0) {
    perror("errore in sendto");
    exit(1);
  } 
    }else if(n == -1){
      perror("error select");
      exit(1);
    }
    //aspetto di ricevere il pkt di risposta da parte del server 
    if (recvfrom(sockfd, &pkt, sizeof(pkt), 0, (struct sockaddr *)&addr,&addrlen) < 0) {
      perror("errore in recvfrom");
      exit(1);
    }
  }
  printf("Client : Server response : %s port number %d\n", pkt.pl, pkt.id);
  fflush(stdout);
  // se ricevo wait ritrasmetto subito una nuova richiesta aspetto un tempo sleep
  //il server ristituisce un pkt con payload wait se tutti i processi che eseguono le richieste del client sono occupati
  if (!strcmp(pkt.pl, "wait")) {
    printf("Server response : wait\n");
    fflush(stdout);
    while (rcv_winy) {
      usleep(400); //aspetto cosi che i processi possano liberarsi
      pkt.code = 2;
      if (temp == 10) { // dopo 10 volte di fila che provo a connettermi esco
        printf("\nClient : Impossibile connettersi con il Server attualmente \n");
        fflush(stdout);
        exit(1);
      }
      temp++;
      //invio una nuova richiesta al server di connessione 
      if (sendto(sockfd, &pkt, sizeof(pkt), 0, (struct sockaddr *)&addr,addrlen) < 0) {
        perror("errore in sendto");
        exit(1);
      }
      n = select(sizeof(fds) * 8, &fds, NULL, NULL, &tv);
      if( n == 0){
        printf("Client : Il Server ha chiuso la connessione\n");
        exit(1);
      }else if( n == -1 ){
        perror("error select");
        exit(1);
      }
      //aspetto di ricevere una risposta 
      if (recvfrom(sockfd, &pkt, sizeof(pkt), 0, (struct sockaddr *)&addr,&addrlen) < 0) {
        perror("errore in recvfrom");
        exit(1);
      }
      printf("Client : Server response : %s port number %d\n", pkt.pl, pkt.id);
      fflush(stdout);
      // se ricevo un pkt con payload go allora ho ottenuto il numero di porta di un processo del server ed esco dal while
      if (!strcmp(pkt.pl, "go")){
        rcv_winy = false;
      }
    }
    }if (!strcmp(pkt.pl,"go")){
      return pkt.id;
    }
    printf("Client : Port number assegnato %d \n", pkt.id);
    fflush(stdout);
  }
// implemento la rcv del comando get
void rcv_get(char *file) {
  free_dim = dim;
  struct st_pkt pkt;
  //alloco  un array per mantenere i pkt che saranno poi usati per la ritrasmissione 
  if ((rcv_win = malloc(sizeof(struct st_pkt) * dim)) == NULL) {
    perror("Error malloc");
    exit(1);
  }
  printf("\n Client : Get function alive\n");
  fflush(stdout);
  FILE *fptr;
  // creo il file se già esiste lo cancello tanto voglio quello aggiornato
  int n = 0, i = 0;
  bool stay = true, different = false;
  char path_file[MAXLINE];
  sprintf(path_file, "Client_Files/%s", file);
  // apro/creo il file 
  if ((fptr= fopen(path_file, "w+")) == NULL) {
    perror("Error opening file");
    exit(1);
  }
  while (stay) {
    // incremento n
    // se differente é false vuol dire che ho ricevuto un pkt in ordine quindi incremento il mio contatore locale n altrimentri non lo incremento e aspetto di ricevere l'id corretto
    if (!different) {
      n++;
    }
    //aspetto di ricevere un pkt dal server 
    if ((recvfrom(sockfd, &pkt, sizeof(pkt), 0, (struct sockaddr *)&addr, &addrlen)) < 0) {
      perror("errore in recvfrom");
      exit(1);
    }
    //  se mi arriva un id che ho già salvato non lo mantengo nell'array
    // se pkt.code == 1 allora chiudo la connessione e pkt.id == n ( ovvero è un pkt con un id che mi aspettavo quindi non è arrivato fuori ordine)
    if (pkt.code == 1 && pkt.id == n) {
      printf("Client Get  last pkt rcvd : id = %d free_dim %d\n", pkt.id, free_dim);
      fflush(stdout);
      rcv_win[i] = pkt;
      i++;
      // cicliclo
      i = i % dim;
      free_dim--;
      // invio subito id 
      pkt.id = n;
      pkt.code = 1;
      pkt.rwnd = free_dim;
      printf("\n Client : Server close connection \n");
      printf("\n Client : Confermo chiusura\n");
      fflush(stdout);
      //invio un pkt per confermare la chiusura
      if (sendto(sockfd, &pkt, sizeof(pkt), 0, (struct sockaddr *)&addr,
                 addrlen) < 0) {
        perror("errore in sendto");
        exit(1);
      }
      // se chiudo e la free_dim è != 0 allora devono scrivere sul file dato che non scrivo subito il payload di un pkt che ho ricevuto ma solo se ho occupato tutto l'array che mantiene i vari pkt(più performante)
      if (free_dim != 0) {
        int t = 0;
        puts("Writing\n");
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
      // se pkt.code = 0 e pkt.id = n allora ho ricevuto un pkt informativo con un id che mi aspettavo lo immagazzino e se free_dim = 0 scrivo tutti i pkt salvati sul file 
    } else if (pkt.code == 0 && pkt.id == n) {
      rcv_win[i] = pkt;
      i++;
      // cicliclo
      i = i % dim;
      different = false;
      free_dim--;
      printf("Client Get pkt rcvd : id = %d free_dim %d\n", pkt.id, free_dim);    
      fflush(stdout);
      pkt.id = n;
      pkt.code = 0;
      pkt.rwnd = free_dim;
      //se free_dim = 0 per prima cosa invio subito la slow per rallentare il server 
      if (free_dim == 0) {
        printf("Client :  send slow to Server\n");
        fflush(stdout);
        strcpy(pkt.pl, "slow");
        pkt.rwnd = dim; // scrivo tutto sul file allora mando come nuova rwnd la dim(il server rallenta la send)
      }
      if (sendto(sockfd, &pkt, sizeof(pkt), 0, (struct sockaddr *)&addr,addrlen) < 0){
        perror("errore in sendto");
        exit(1);
      }
      //scrivo i dati ricevuto sul file 
      if (free_dim == 0){
        int t = 0;
        free_dim = dim;
        for (t = 0; t < dim; t++) {
          if ((fwrite(rcv_win[t].pl, strlen(rcv_win[t].pl), 1, fptr) < 0)) {
            perror("Error in write rcv_get\n");
            exit(1);
          }
        }
        printf("Client write all pkt\n");
        fflush(stdout);
      }
    }
    // se arriva un pkt fuori ordine invio subito id non faccio la
    // bufferizzazione lato rcv
    else if (stay == true && pkt.id != n) {
      // non incremento n pongo diff = true
      different = true;
      // invio al sender un id comulativo fino a dove ho ricevuto
      printf("Client : pkt fuori ordine num %d  invio id %d \n", pkt.id, n - 1);
      fflush(stdout);
      pkt.id = n - 1;
      pkt.rwnd = free_dim;
      if (sendto(sockfd, &pkt, sizeof(pkt), 0, (struct sockaddr *)&addr,addrlen) < 0) {
        perror("errore in sendto");
        exit(1);
      }
    }
  }
  //chiudo-free i vari puntatori
  fclose(fptr);
  free(rcv_win);
}
//mretr: implementa la ritrasmissione dei pkt che il client ha inviato al server
void *mretr() {
  s = true;
  while (s) {
    puts("timeout started\n");
    //implemento il timeout
    usleep(dynamics_timeout);
    if (lt_ack_rcvd != seqnum) {
      puts("timeout finished\n");
      rit = true;
      struct st_pkt pkt;
      int k;
      // dimezzo la ConWin
      if (CongWin > 1) {
        CongWin = CongWin>>1;
      }
      k = lt_ack_rcvd;
      //implemento la ritrasmissione di tutti i pkt dopo lt_ack_rcvd
      swnd = 0;
      // ritrasmetto tutti i pkt dal pkt con id=last_ack_rcvd  fino all'ultimo pkt trasmesso
      while (k < seqnum) {
        while (swnd < CongWin && swnd < lt_rwnd && k < seqnum) {
          if ((sendto(fd, &rcv_win[k], sizeof(pkt), 0, (struct sockaddr *)&addr,
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
   int sockfd = *(int*)sd;
  fd = *(int*)sd;
  struct st_pkt pkt;
  int t = 0;
  pthread_t thread_id;
  lt_ack_rcvd = 0;
  bool stay = true;
  //creo il thread per la ritrasmissione 
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
    printf("Ricevuto pkt: id %d lt_rcvd %d swnd:%d\n", pkt.id, lt_ack_rcvd,swnd);
    fflush(stdout);
    //entro nell'if solo se il pkt ricevuto ha un id nuovo quindi non è arrivato fuori ordine
    if (pkt.id > lt_ack_rcvd ){
      id_dup=false;
      //se adpt_timeout=true sole se l'utente ha inserito l'opzione di timeout dynamic
      if(adpt_timeout){
      dynamics_timeout=dynamics_timeout+500; //timeout dynamic
      }
      CongWin++;
      num = pkt.id + 1;
      lt_ack_rcvd = pkt.id;
      lt_rwnd = pkt.rwnd; //assegno un nuovo valore alla lt_rwnd che tiene conto della dimensione libera del buffer del server 
      // se ricevo un pkt con payload = slow allora rallento in quanto il server sta scrivendo sul file
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
        s=false;  //blocco il while della ritrasmissione
        return NULL;
      }

    } else {
      // se ricevo un id duplicato allora imposto id_dup = true cosi da bloccare la trasmissione dei in questo caso della snd_put in quanto mi rendo conto che tutti i nuovi pkt inviati andranno comunque persi poichè arriveranno fuori ordine.
      if(pkt.id != lt_ack_rcvd ) 
      id_dup=true;
       if(dynamics_timeout/2>timeout){
      dynamics_timeout>>1;
      }else{
        dynamics_timeout = timeout;
      }  
      }  
    }
      s = false;
   // aspetto la terminazione del thread che legge
  if (pthread_join(thread_id, NULL) != 0) {
    perror("Error pthread_join");
    exit(1);
  }
}
// implemento la snd del comando put
void snd_put(char *str, int sockfd) {
  /* VALUTAZIONE PRESTAZIONI*/
  struct timeval begin, end;
  gettimeofday(&begin, 0);
  ///
  printf("\nClient : Put alive \n");
  fflush(stdout);
  // variabili 
  seqnum = 0;
  lt_ack_rcvd = 0;
  swnd = 0;
  CongWin = 1;
  dim = 0;
  lt_rwnd = 1;
  int size = 0;
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
  pthread_t thread_id;
  //apro il file da leggere
  sprintf(path_file, "Client_Files/%s", str);
  if ((file = fopen(path_file, "r+")) == NULL) {
    printf("Errore in open del file\n");
    exit(1);
  }
  // ottengo la dimesione del file cosi da definire la dimensione
  fseek(file, 0, SEEK_END);     // seek to end of file
  size = ftell(file);           // get current file pointer
  fseek(file, 0, SEEK_SET);     // seek back to beginning of file
  dim = ((size) / MAXLINE) + 1; // +1 perchè arrotonda per difetto
  //alloco l'array per mantenere i pkt inviati, array che sarà utilizzato per la ritrasmissione
  if ((rcv_win = malloc(sizeof(struct st_pkt) * dim)) == NULL) {
    perror("Error malloc");
    exit(1);
  }
  // creo il thread che implementa la ricezione e il loro controllo
  if (pthread_create(&thread_id, NULL, rcv_cong, &sockfd) != 0) {
    perror("error pthread_create");
    exit(1);
  }
  while (stay) {
    // se last id non è uguale al mio seqnum mi fermo altrimenti entro dentro e se non sto ritrasmettendo i pkt 
    if (lt_ack_rcvd == seqnum && stay == true && !rit) {
      // se il numero di pkt in volo è < della dimensione della CongWin e < dello spazio ancora libero in memoria lato server ed non sto ritrasmettendo e non ho ricevuto un id duplicato entro 
      while (swnd < CongWin && stay == true && swnd < lt_rwnd && !rit && !id_dup) {
        //leggo il file finchè la fread non legge meno di MAXLINE in questo caso sono arrivato a fine file
        if ((dimpl = fread(pkt.pl, 1, sizeof(pkt.pl), file)) == MAXLINE) {
          seqnum++; //aumento il numero di pkt 
          swnd++;   //aumento il numero di pkt in "volo"
          pkt.code = 0; //imposto il pkt come informativo 
          pkt.id = seqnum; 
          //mantengo il pkt nell'array
          rcv_win[i] = pkt;
          i++;
          prob = (double)rand() / RAND_MAX; //implemento la probabilità di errore
          if (prob < p) { //implemento l'errore
            msgPerso++;
            msgTot++;
            // Il messaggio è stato perso
          printf(" LOST :: ACK = %d  swnd = %d CongWin = %d  lt_rwnd = %d\n",
                 pkt.id, swnd, CongWin, lt_rwnd);
            continue;
          } else {
            // Trasmetto con successo
            /*
            if (lt_ack_rcvd < seqnum - 10) { // se lt_ack è molto piccolo rallento(flow_control)
              // rallento flow control
              bytes_psecond = 10;
              if (setsockopt(sockfd, SOL_SOCKET, SO_MAX_PACING_RATE,&bytes_psecond, sizeof(bytes_psecond)) < 0) {
                perror("Error setsockopt");
                exit(1);
              }
            } else {
              bytes_psecond = 100;
              if (setsockopt(sockfd, SOL_SOCKET, SO_MAX_PACING_RATE,&bytes_psecond, sizeof(bytes_psecond)) < 0) {
                perror("Error setsockopt");
                exit(1);
              }
            }
            */
            if ((sendto(sockfd, &pkt, sizeof(pkt), 0, (struct sockaddr *)&addr,addrlen)) < 0) {
              perror("errore in sendto");
              exit(1);
            }
            msgInviati++;
            msgTot++;
          }
        } else if (feof(file)) {  //entro solo se ho letto tutto il file
          while (lt_ack_rcvd != seqnum) { //aspetto di sincronizzare last ack rcvd con il numero associato all'ultimo pkt trasmesso
            usleep(200);//sleep cosi da non rimanere schedulato
          }
          seqnum++; //aumento il seqnum 
          pkt.id = seqnum; 
          pkt.code = 1; //pkt.cod=1 pkt di chiusura ho letto tutto il file 
          pkt.pl[dimpl] = '\0'; // payload vuoto 
          rcv_win[i] = pkt;//mantengo il pkt per la ritrasmissione
          stay = false;
          swnd++; //incremento il numero di pkt in volo
          printf("\nServer: send last pkt %d  \n ",seqnum);
          prob = (double)rand() / RAND_MAX; //implemento la probabilità di errore
          if (prob < p) {
            // Il messaggio è stato perso
            msgPerso++;
            msgTot++;
          } else {
            // invio il terminatore
            printf("\n Server : Close connection \n");
            fflush(stdout);
            if (sendto(sockfd, &pkt, sizeof(pkt), 0, (struct sockaddr *)&addr,addrlen) < 0) {
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
  printf("\n PROB DI SCARTARE UN MSG %f\n", p);
  printf("\n TIMEOUT FINALE  %d\n", dynamics_timeout);
  fflush(stdout);
  //libero-chiudo i vari descrittori
  free(rcv_win);
  fclose(file);
}
// implemento la rcv del comando list
void rcv_list() {
  free_dim = dim; //valore di dim impostato dall'utente per maggiore flessibilità
  struct st_pkt pkt;
  //alloco l'array che mantiene i pkt
  if ((rcv_win = malloc(sizeof(struct st_pkt) * dim)) == NULL) {
    perror("Error malloc");
    exit(1);
  }
  printf("\n Client : List function alive\n");
  fflush(stdout);
  int n = 0, i = 0,k = 0;
  bool stay = true, different = false;
  while (stay) {
    // incremento n se ho ricevuto un pkt con un id nuovo 
    if (!different) {
      n++;
    }
    //aspetto di ricevere i pkt dal server
    if ((recvfrom(sockfd, &pkt, sizeof(pkt), 0, (struct sockaddr *)&addr,&addrlen)) < 0) {
      perror("errore in recvfrom");
      exit(1);
    }
    // se pkt.code == 1  e pkt.id == n allora chiudo la connessione(pkt di chiusura e con id che mi aspettavo) 
    if (pkt.code == 1 && pkt.id == n) {
      printf("\nClient : last pkt :  id = %d free_dim %d\n", pkt.id, free_dim);
      fflush(stdout);
      printf("\n Client : Server close connection \n");
      rcv_win[i] = pkt;
      i++;
      // cicliclo
      i = i % dim;
      free_dim--;
      pkt.id = n;
      pkt.code = 1;//invio un pkt di chiusura confermo la chiusura al server
      pkt.rwnd = free_dim;
      printf("\n Client : Confermo chiusura\n");
      fflush(stdout);
      //invio il pkt
      if (sendto(sockfd, &pkt, sizeof(pkt), 0, (struct sockaddr *)&addr,addrlen) < 0) {
        perror("errore in sendto");
        exit(1);
      }
      printf("Client : List -> ::\n ");
      //stampo a schermo tutti i nomi dei file, ricevuti dal server
      if (free_dim != 0) {
        int t = 0;
        for (t = 0; t < dim - free_dim; t++) {
          printf("%s\n", rcv_win[t].pl);
        }
        puts("\n");
      }
      stay = false;
      // se il code è 0 e il numero di pkt è quello che mi aspettavo mantengo il pkt nella rcv_win e invio un pkt al server per il riscontro
    } else if (pkt.code == 0 && pkt.id == n) {
      rcv_win[i] = pkt;
      i++;
      // cicliclo
      i = i % dim;
      different = false;  //posso incrementare n
      free_dim--;
      printf("Client : pkt ricevuto : id = %d free_dim %d\n", pkt.id, free_dim);
      fflush(stdout);
      pkt.id = n;
      pkt.code = 0; //pkt informativo
      pkt.rwnd = free_dim;
      if (free_dim == 0) {  //solo se la rcv_win è piena scrivo sul file cosi da migliorare in generale le prestazioni del processo
        strcpy(pkt.pl, "slow");
        pkt.rwnd = dim;
        printf("Client : send slow to Server\n");
        fflush(stdout); 
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
    // se arriva un pkt fuori ordine invio subito id non faccio la
    // bufferizzazione lato rcv
    else if (stay == true && pkt.id != n) {
      // non incremento n pongo diff = true
      different = true;
      // invio al sender un id comulativo fino a dove ho ricevuto
      pkt.id = n - 1;
      pkt.rwnd = free_dim;
      if (sendto(sockfd, &pkt, sizeof(pkt), 0, (struct sockaddr *)&addr,addrlen) < 0) {
        perror("errore in sendto");
        exit(1);
      }
    }
  }
  free(rcv_win);
}
// command_send: funzione principale che crea il pkt di richiesta, il quale sarà inviato al processo child del server
void command_send(char *cd, char *nome_str) {
  struct st_pkt pkt;
  int temp = 0;
  pkt.id = 0;
  char str[MAXLINE];
  int i = 0;
  fd_set fds;
  struct timeval tv;
  int len = 0, u = 0;
  bool r = true;
  char buffer[MAXLINE];
  // Set up the file descriptor set.
  FD_ZERO(&fds);
  FD_SET(sockfd, &fds);
  tv.tv_usec = 0; // ms waiting
  tv.tv_sec = 10; // s waiting
  strcpy(str, cd);
  if (nome_str != NULL) {
    strcat(str + strlen(cd), nome_str);
  }
  printf("\n nome comando %s\n", str);
  fflush(stdout);
  strcpy(pkt.pl, str);
  pkt.code = 0;
  pkt.id = 0;
  // Invia al server il pacchetto di richiesta al server
  if (sendto(sockfd, &pkt, sizeof(pkt), 0, (struct sockaddr *)&addr, addrlen) < 0) {
    perror("errore in sendto");
    exit(1);
  }
  pkt.id = -2;
  while (pkt.id != 0 && pkt.id != -1){
    i = select(sizeof(fds) * 8, &fds, NULL, NULL, &tv); //implemeto la select cosi da gestire  i diversi casi
    if (i == 0) {
      //se la select scade e quindi non ho ricevuto nessun pkt dal server per 10 s esco il server è off
      printf("Client : Server disconesso riprovare \n");
      exit(1);
    } else if (i == -1) {
      perror("error select");
      exit(1);
    } else {
      // Legge dal socket il pacchetto di risposta
      if (recvfrom(sockfd, &pkt, sizeof(pkt), 0, (struct sockaddr *)&addr,&addrlen) < 0) {
        perror("errore in recvfrom");
        exit(1);
      }
      if (pkt.id == -1) { //errore 
        printf("\n Error Server = %s\n", pkt.pl);
        fflush(stdout);
        req(); // gestisco possibili errori
        loop = true;  //gestisco il caso in cui rientro nella req più volte si crea un loop che viene gestisco da questo bool
        return;
      } else if (pkt.id == temp) {  //entro se il server risponde correttamente
        printf("\n Server response : %s\n", pkt.pl);
        fflush(stdout);
        // implento la list
        if (!strcmp(cd, "list ")) {
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
      }else{
        printf("Client : ho ricevuto un pkt sospetto chiudo\n");
        exit(1);
      }
      // leggo tutti i byte nella socket mi serve nel caso in cui il client
      // vuole comunicare nuovamente con la stessa socket
    }
    while (r) { // leggo tutto quello che è rimasto nella socket gestisco eventuali ritrasmissioni ritardate di pkt
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
}
// gestisco la richiesta dell'utente
void req() {
  struct st_pkt pkt;
  pkt.id = 0;
  pkt.code = 0;
  int a = 0, temp = 0, t = 0,y=0;
  char buff[MAXLINE];
  while (1) {
    a = 0, temp = 0, t = 0,buff[0]='\0';  //resetto i valori dei parametri in caso di richieste cicliche 
    // se ho gestito un errore e si è creato un loop chiudo
    if (loop) {
      break;
    }
    temp = port_number(sockfd); //richiedo il numero di porta al server
    addr.sin_port = htons(temp); // assegna la porta presa dal server
    printf("\nInserire numero:\nget = 0\nlist = 1\nput = 2\nexit = -1\n");
    if (y=(fscanf(stdin, "%d", &a)) == EOF) {
        perror("Error fscanf");
        exit(1);
      }
      //gestisco il caso in cui la scanf riceve un elemento diverso dalla tipologia richiesta(uguale per l'implementazioni a seguire con la y)
      if(y < 1){
        fgets(buff,MAXLINE,stdin);
        buff[0]='\0';
      }
    if (a == -1) { // chiudo subito senza che invio il pkt al server
      pkt.id = 0;
      pkt.code= 0;
      strcpy(pkt.pl,"quit ");
      printf("Client : send close pkt to server\n");
      fflush(stdout);
      if (sendto(sockfd, &pkt, sizeof(pkt), 0, (struct sockaddr *)&addr, addrlen) < 0) {
      perror("errore in sendto");
      exit(1);
    } 
      return;
    }
    if ((recvfrom(sockfd, &pkt, sizeof(pkt), MSG_DONTWAIT,(struct sockaddr *)&addr, &addrlen)) < 0) {
      if (errno == EAGAIN) {
      } else {
        perror("errore in recvfrom");
        exit(-1);
      }
    }
    //se ricevo code = 3 e il payload è Close vuol dire che il server si è disconesso perchè il pkt ha impiegato troppo tempo ad arrivare(probabile congestione della rete)
    if (pkt.code == 3 && !strcmp(pkt.pl, "Close")) {
      printf("Client : Server close connection\n");
      temp = port_number(sockfd);//richiedo il numero di porta 
      addr.sin_port = htons(temp); // assegna la porta presa dal server
    }
    // gestisco il caso in cui il client inserisce un numero diverso da quello
    // desiderato
    while (a != 0 && a != 1 && a != 2 && a != -1) {
      t++;
      printf("\nInserire numero:\nget = 0\nlist = 1\nput = 2\nexit = -1\n");
      if (y=(fscanf(stdin, "%d", &a)) == EOF) {
        perror("Error fscanf");
        exit(1);
      }
      //vedere sopra dove implemento per la prima volta tale gestione dell'errore
      if(y < 1){
        fgets(buff,MAXLINE,stdin);
        buff[0]='\0';
      }
      if (t > 5) {
        printf("\nClient : troppi tentativi falliti riavviare il processo\n");
        exit(1);
      }
    }
    t=0;
    //switch per gestire le varie opzioni che l'utente può scegliere 
    switch (a) {
    case 0:
      printf("\nInserire dimensione buff\n");
      if (y=(fscanf(stdin, "%d", &dim)) == EOF) {
        perror("Error fscanf");
        exit(1);
      }
      if(y < 1){
        fgets(buff,MAXLINE,stdin);
        buff[0]='\0';
      }
      while(dim < 10 || dim > 10000){
        t++;
        printf("inserire un valore della dimensione del buffer compreso tra 10 e 1000\n");
        if (y=(fscanf(stdin, "%d", &dim)) == EOF) {
        perror("Error fscanf");
        exit(1);
      }
      if(y < 1){
        fgets(buff,MAXLINE,stdin);
        buff[0]='\0';
      }
      if (t > 5) {
        printf("\nClient : troppi tentativi falliti riavviare il processo\n");
        exit(1);
      }
      }
      t=0;
      free_dim = dim;
      printf("\nInserire nome file\n");
      if ((temp = read(0, buff, sizeof(buff))) < 0) {
        perror("Error read");
        exit(1);
      }
      buff[temp - 1] = '\0';
      command_send("get ", buff);
      break;

    case 1:
      printf("\nInserire dimensione buff\n");
      if (y=(fscanf(stdin, "%d", &dim)) == EOF) {
        perror("Error fscanf");
        exit(1);
      }
      if(y < 1){
        fgets(buff,MAXLINE,stdin);
        buff[0]='\0';
      }
      while(dim < 1 || dim > 10000){
        t++;
        printf("inserire un valore della dimensione del buffer compreso tra 1 e 1000\n");
        if (y=(fscanf(stdin, "%d", &dim)) == EOF) {
        perror("Error fscanf");
        exit(1);
      }
      if(y < 1){
        fgets(buff,MAXLINE,stdin);
        buff[0]='\0';
      }
      if (t > 5) {
        printf("\nClient : troppi tentativi falliti riavviare il processo\n");
        exit(1);
      }
      }
      t=0;
      free_dim = dim;
      command_send("list ",NULL); // passo direttamento la list alla command_send senza usare una funzione ausiliaria
      break;
    case 2:
      char te[100]; 
      system("mkdir Client_Files"); //creo la directory dove voglio immagazzinare il file
      //timeout dinamico ?
      printf("\n< timeout adattivo s = si n = no >\n");
      if(fgets(te,100,stdin) == NULL ){
      perror("errore fgets");
      exit(1);
    }
     if(!strcmp(te,"s")){
    adpt_timeout=true;
  }else if(!strcmp(te,"n")){
    adpt_timeout=false;
  }
    //controllo timeout adattivo
    while(strcmp(te,"s") && strcmp(te,"n")){
    t++;
    te[0]='\0';
    if(fgets(te,4,stdin) == NULL ){
      perror("errore fgets");
      exit(1);
    }
    te[1]='\0';
    if(!strcmp(te,"s")){
    adpt_timeout=true;
  }else if(!strcmp(te,"n")){
    adpt_timeout=false;
  }
  if (t > 5) {
      printf("Inserito troppe volte il valore sbagliato\n");
      exit(1);
    }
  }
  t=0;
      //ottengo il valore del Timeout
      printf("\nInserire Timeout in us\n");
      if (y=(fscanf(stdin, "%d", &timeout)) == EOF) {
        perror("Error fscanf");
        exit(1);
      }
      if(y < 1){
        fgets(buff,MAXLINE,stdin);
        buff[0]='\0';
      }
        //controllo sul valore del timeout
    while(timeout < 10000 || timeout > 120000000 ){
      t++;
      printf("inserire un timeout 10ms < timeout < 120s\n");
      if (y=(fscanf(stdin, "%d", &timeout)) == EOF) {
        perror("Error fscanf");
        exit(1);
      }
      if(y < 1){
        fgets(buff,MAXLINE,stdin);
        buff[0]='\0';
      }
    if (t > 5) {
      printf("Inserito troppe volte il numero sbagliato\n");
      exit(1);
    }
    }
    t=0;
      dynamics_timeout=timeout;
      //ottengo il valore della probabilità di errore
      printf("\nInserire p \n");
      if (y=(fscanf(stdin, "%le", &p)) == EOF) {
        perror("Error fscanf");
        exit(1);
      }
      if(y < 1){
        fgets(buff,MAXLINE,stdin);
        buff[0]='\0';
      }
      //controllo sul valore della probabilità di errore
      while( p > 1 || p < 0){
    t++;
    printf("inserire una probabilità di errore compresa tra 0 e 1\n");
    if (y=(fscanf(stdin, "%le", &p)) == EOF) {
        perror("Error fscanf");
        exit(1);
      }
      if(y < 1){
        fgets(buff,MAXLINE,stdin);
        buff[0]='\0';
      }
    if (t > 5) {
      printf("Inserito troppe volte il numero sbagliato\n");
      exit(1);
    }
  }
  t=0;
      printf("\nInserire nome file\n");
      if ((temp = read(0, buff, sizeof(buff))) < 0) {
        perror("Error fread");
        exit(1);
      }
      buff[temp - 1] = '\0';
      command_send("put ", buff);
      break;
    case -1:
    //implemento la chisura del client il quale invierà un pkt al server per liberare il processo lato server
      pkt.id = 0;
      pkt.code=0;
      strcpy(pkt.pl,"quit ");
      printf("Client : send close pkt to server\n");
      fflush(stdout);
      if (sendto(sockfd, &pkt, sizeof(pkt), 0, (struct sockaddr *)&addr, addrlen) < 0) {
      perror("errore in sendto");
      exit(1);
    } 
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
  if (sigaction(signum, &act, &oact) < 0){
    return (SIG_ERR);
  }
  return (oact.sa_handler);
}
void sig_int(int signo) { exit(1); }

int main(int argc, char *argv[]) {
  if (argc != 3) { // controlla numero degli argomenti
    fprintf(stderr, "utilizzo: <indirizzo IP server> <numero di porta uguale al server >\n");
    exit(1);
  }
  if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) { // crea il socket
    perror("errore in socket");
    exit(1);
  }
  SERV_PORT = atoi(argv[2]);
  memset((void *)&addr, 0, sizeof(addr)); // azzera addr
  addr.sin_family = AF_INET;              // assegna il tipo di indirizzo
  addr.sin_port = htons(SERV_PORT);       // assegna il numero di porta
  // assegna l'indirizzo del server prendendolo dalla riga di comando.
  // L'indirizzo è una stringa da convertire in intero secondo network byte order.
  if (inet_pton(AF_INET, argv[1], &addr.sin_addr) <= 0) {
    // inet_pton (p=presentation) vale anche per indirizzi IPv6
    fprintf(stderr, "errore in inet_pton per %s", argv[1]);
    exit(1);
  }
  if (signal(SIGINT, sig_int) == SIG_ERR) {
    fprintf(stderr, "errore in signal INT ");
    exit(1);
  }
  // invoco la funzione per gestire le richieste dell'utente
  req();
}
