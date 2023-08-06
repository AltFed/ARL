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
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/syscall.h>
#include <time.h>
#include <unistd.h>
#include <wait.h>
#include <sys/ioctl.h>
#define MAXLINE 4096
// implementare un mutex per prendere il controllo e comunicare con il proprio client dato che i thread modificano le variabili globali !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
//!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
// variabili globali
int TOs = 0;
int TOms = 0;
int seqnum = 0;
int lt_ack_rcvd = 0;
int lt_rwnd = 1;
int swnd = 0;
int CongWin = 1;
double p = 0;
int maxackrcv = 0;
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

// implementa il controllo di segnali per gestire i thread
typedef void Sigfunc(int);

// pkt struct
struct st_pkt
{
  int ack;
  int finbit;
  char pl[MAXLINE];
  int rwnd;
};
// array per mantenere i pkt
struct st_pkt *retr;
// rcv list
void *rcv_list(void *sd)
{
  
}
// thread per la gestione del ack cum
void *rcv_cong(void *sd)
{
  printf("RCV START\n");
  fflush(stdout);
  int sockfd = sd;
  struct st_pkt pkt;
  int k = 0,temp = 0, n;
  // Wait until timeout or data received.
  bool stay = true;
  while (stay)
  {
    if (lt_ack_rcvd == k)
    {
      alarm(2);
    }
    if (recvfrom(sockfd, &pkt, sizeof(pkt), 0, (struct sockaddr *)&addr, &addrlen) < 0)
    {
      if(errno == EINTR){
        puts("TO interrotto rcvfrom\n");
      }
      perror("errore in recvfrom");
      exit(1);
    }
    if (pkt.ack > lt_ack_rcvd)
    {
      alarm(0);
      lt_ack_rcvd = pkt.ack;
      k = lt_ack_rcvd;
      CongWin++;
      swnd = seqnum - lt_ack_rcvd;
      lt_rwnd = pkt.rwnd;
      lt_ack_rcvd = pkt.ack;
      // se è un ack nuovo entro qui
      //  se non ho errore allora leggo e vedo l ack che il receiver mi invia
      if (!strcmp(pkt.pl, "slow"))
      {
        printf("\nSLOW RCVD\n");
        fflush(stdout);
        bytes_psecond = 100;
        if (setsockopt(sockfd, SOL_SOCKET, SO_MAX_PACING_RATE,
                       &bytes_psecond, sizeof(bytes_psecond)) < 0)
        {
          perror("Error setsockopt");
          exit(1);
        }
      }
      else
      {
        bytes_psecond = 1000;
        if (setsockopt(sockfd, SOL_SOCKET, SO_MAX_PACING_RATE,
                       &bytes_psecond, sizeof(bytes_psecond)) < 0)
        {
          perror("Error setsockopt");
          exit(1);
        }
      }
      if (pkt.finbit == 1 && pkt.ack == seqnum)
      {
        printf("Server : Client disconesso  correttamente \n");
        fflush(stdout);
        stay = false;
        return NULL;
      }
    }
  }
}

// gestice nello specifico il comando get
void send_get(char *str, int sockfd)
{
  // variabili da resettare
  seqnum = 0;
  lt_ack_rcvd = 0;
  swnd = 0;
  CongWin = 1;
  maxackrcv = 0;
  dim = 0;
  int size = 0;
  printf("\nSend_get\n");
  fflush(stdout);
  struct st_pkt pkt;
  FILE *file;
  bool stay = true;
  int i = 0, msgInviati = 0, msgPerso = 0, msgTot = 0, dimpl = 0;
  double prob = 0;
  char path_file[MAXLINE];
  sprintf(path_file, "Server_Files/%s", str);
  if ((file = fopen(path_file, "r+")) == NULL)
  {
    printf("Errore in open del file\n");
    exit(1);
  }
  // ottengo la dimesione del file cosi da definire la dim
  fseek(file, 0, SEEK_END);     // seek to end of file
  size = ftell(file);           // get current file pointer
  fseek(file, 0, SEEK_SET);     // seek back to beginning of file
  dim = ((size) / MAXLINE) + 1; // +1 perchè arrotonda per difetto
  if ((retr = malloc(sizeof(struct st_pkt) * dim)) == NULL)
  {
    perror("Error malloc");
    exit(1);
  }
  printf("\ndim %d\n", dim);
  pthread_t thread_id;
  // creo il thread che mi legge le socket (lettura bloccante)
  if (pthread_create(&thread_id, NULL, rcv_cong, sockfd) != 0)
  {
    perror("error pthread_create");
    exit(1);
  }
  while (stay)
  {
    // se last ack non è uguale al mio seqnum mi fermo altrimenti entro dentro e
    // invio da 0 a CongWin pkt e poi mi aspetto di ricevere come lastack quello
    // dell'ultimo pkt inviato poi continuo
    if (lt_ack_rcvd == seqnum && stay == true)
    {
      while (swnd < CongWin && stay == true && swnd < lt_rwnd)
      {
        printf(" SEND_GET :: swnd = %d CongWin = %d  lt_rwnd = %d\n", swnd, CongWin, lt_rwnd);
        fflush(stdout);
        if ((dimpl = fread(pkt.pl, 1, sizeof(pkt.pl), file)) == MAXLINE)
        {
          seqnum++;
          swnd++;
          pkt.finbit = 0;
          pkt.ack = seqnum;
          // mantengo CongWin pkt
          retr[i] = pkt;
          printf("\nack %d\n", pkt.ack);
          fflush(stdout);
          i++;
          // cicliclo
          i = i % dim;
          // aumento la dim del vettore che mi salva i pkt
          prob = (double)rand() / RAND_MAX;
          if (prob < p)
          {
            msgPerso++;
            msgTot++;
            swnd++;
            // Il messaggio è stato perso
            continue;
          }
          else
          {
            // Trasmetto con successo
            if (lt_ack_rcvd < seqnum - 5)
            {
              // rallento flow control
              bytes_psecond = 10;
              if (setsockopt(sockfd, SOL_SOCKET, SO_MAX_PACING_RATE,
                             &bytes_psecond, sizeof(bytes_psecond)) < 0)
              {
                perror("Error setsockopt");
                exit(1);
              }
            }
            else
            {
              bytes_psecond = 100;
              if (setsockopt(sockfd, SOL_SOCKET, SO_MAX_PACING_RATE,
                             &bytes_psecond, sizeof(bytes_psecond)) < 0)
              {
                perror("Error setsockopt");
                exit(1);
              }
            }
            if ((sendto(sockfd, &pkt, sizeof(pkt), 0, (struct sockaddr *)&addr, addrlen)) < 0)
            {
              perror("errore in sendto");
              exit(1);
            }
            msgInviati++;
            msgTot++;
          }
          // la lettura la fa il thread cosi non mi blocco io main thread
        }
        else if (feof(file))
        {
          while (lt_ack_rcvd != seqnum)
          {
            // Aspetto che il thread legga last ack
            usleep(5);
          }
          printf("fine file\n");
          fflush(stdout);
          seqnum++;
          pkt.ack = seqnum;
          pkt.finbit = 1;
          pkt.pl[dimpl] = '\0';
          retr[i] = pkt;
          printf("\n\n\nSERVER send last pkt %d %s  ", seqnum, pkt.pl);
          prob = (double)rand() / RAND_MAX;
          printf("\n Server : Close connection \n");
          fflush(stdout);
          if (prob < p)
          {
            // Il messaggio è stato perso
            msgPerso++;
            msgTot++;
            swnd++;
            stay = false;
            continue;
          }
          else
          {
            stay = false;
            // invio il terminatore
            if (sendto(sockfd, &pkt, sizeof(pkt), 0, (struct sockaddr *)&addr,
                       addrlen) < 0)
            {
              perror("Error in sendto");
              exit(1);
            }
            msgInviati++;
            msgTot++;
            swnd++;
          }
        }
        else if (ferror(file))
        {
          perror("Error fread ");
          exit(1);
        }
      }
    }
  }
  // aspetto la terminazione del thread che legge
  if (pthread_join(thread_id, NULL) != 0)
  {
    perror("Error pthread_join");
    exit(1);
  }
  printf("\nMSG TOTALI %d\n", msgTot);
  printf("\nMSG PERSI %d\n", msgPerso);
  printf("\nMSG INVIATI %d\n", msgInviati);
  printf("\n Dim CongWin finale %d\n", CongWin);
  printf("\n PROB DI SCARTARE UN MSG %f\n", p);
  fflush(stdout);
  free(retr);
  fclose(file);
}

// gestice nello specifico il comando put
void rcv_put(char *file, int sockfd)
{
  dim = 100;
  free_dim = dim;
  struct st_pkt pkt;
  if ((retr = malloc(sizeof(struct st_pkt) * dim)) == NULL)
  {
    perror("Error malloc");
    exit(1);
  }
  printf("\n Client : Get function alive\n");
  fflush(stdout);
  FILE *fptr;
  // creo il file se già esiste lo cancello tanto voglio quello aggiornato
  int n = 0, i = 0;
  bool stay = true, different = false;

  if ((fptr = fopen(file, "w+")) == NULL)
  {
    perror("Error opening file");
    exit(1);
  }
  while (stay)
  {
    // incremento n
    if (!different)
    {
      n++;
    }
    if ((recvfrom(sockfd, &pkt, sizeof(pkt), 0, (struct sockaddr *)&addr,
                  &addrlen)) < 0)
    {
      perror("errore in recvfrom");
      exit(1);
    }
    printf("ack rcvd  %d\n\n ", pkt.ack);
    // se mi arriva un ack che ho già salvato non lo mantengo nell'array
    // finbit == 1 allora chiudo la connessione
    if (pkt.finbit == 1 && pkt.ack == n)
    {
      printf("\nPL %s", pkt.pl);
      retr[i] = pkt;
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
      printf("\nACK --> %d N %d\n", pkt.ack, n);
      fflush(stdout);

      if (sendto(sockfd, &pkt, sizeof(pkt), 0, (struct sockaddr *)&addr, addrlen) < 0)
      {
        perror("errore in sendto");
        exit(1);
      }
      // se mi arriva come unico pkt un pkt di fine rapporto chiudo e scrivo sul
      // file però altrimenti non scrivo
      if (free_dim != 0)
      {
        int t = 0;
        for (t = 0; t < dim - free_dim; t++)
        {
          printf("Indice %d - PL : %s\n", t, retr[t].pl);
          fflush(stdout);
          if ((fwrite(retr[t].pl, strlen(retr[t].pl), 1, fptr) < 0))
          {
            perror("Error in write rcv_get\n");
            exit(1);
          }
        }
      }
      stay = false;
      // se il finbit è 0 e il numero di pkt è quello che mi aspettavo scrivo
      // sul file il pkt ricevuto
    }
    else if (pkt.finbit == 0 && pkt.ack == n)
    {
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
      if (free_dim == 0)
      {
        printf("Client send slow to Server\n");
        fflush(stdout);
        strcpy(pkt.pl, "slow");
      }
      if (sendto(sockfd, &pkt, sizeof(pkt), 0, (struct sockaddr *)&addr,
                 addrlen) < 0)
      {
        perror("errore in sendto");
        exit(1);
      }
      if (free_dim == 0)
      {
        int t = 0;
        free_dim = dim;
        for (t = 0; t < dim; t++)
        {
          if ((fwrite(retr[t].pl, strlen(retr[t].pl), 1, fptr) < 0))
          {
            perror("Error in write rcv_get\n");
            exit(1);
          }
        }
        pkt.rwnd = free_dim;
        strcpy(pkt.pl, "\0");
        if (sendto(sockfd, &pkt, sizeof(pkt), 0, (struct sockaddr *)&addr,
                   addrlen) < 0)
        {
          perror("errore in sendto");
          exit(1);
        }
      }
    }
    // se arriva un pkt fuori ordine invio subito ack non faccio la
    // bufferizzazione lato rcv
    else if (stay == true && pkt.ack != n)
    {
      // non incremento n pongo diff = true
      different = true;
      // invio al sender un ack comulativo fino a dove ho ricevuto
      printf("NO num %d  invio ack %d \n", pkt.ack, n - 1);
      fflush(stdout);
      pkt.ack = n - 1;
      pkt.rwnd = free_dim;
      if (sendto(sockfd, &pkt, sizeof(pkt), 0, (struct sockaddr *)&addr,
                 addrlen) < 0)
      {
        perror("errore in sendto");
        exit(1);
      }
    }
  }
  fclose(fptr);
  free(retr);
}

// gestice nello specifico il comando list
void send_list(int sockfd)
{
  seqnum = 0;
  lt_ack_rcvd = 0;
  swnd = 0;
  CongWin = 1;
  maxackrcv = 0;
  dim = 10;
  struct st_pkt pkt;
  DIR *directory;
  struct dirent *file;
  bool stay = true;
  int i = 0, msgInviati = 0, msgPerso = 0, msgTot = 0, dimpl = 0, k = 0, temp = 0;
  double prob = 0;
  pthread_t thread_id;
  if ((retr = malloc(sizeof(struct st_pkt) * dim)) == NULL)
  {
    perror("Error malloc");
    exit(1);
  }
  printf("Server : Send_list Alive\n");
  printf("swnd %d lt %d seq %d rwnd %d congwin %d\n", swnd, lt_ack_rcvd, seqnum, lt_rwnd, CongWin);
  fflush(stdout);
  // creo il thread che mi legge le socket
  if (pthread_create(&thread_id, NULL, rcv_list, sockfd) != 0)
  {
    perror("error pthread_create");
    exit(1);
  }
  // Apertura della cartella
  directory = opendir("Server_Files");

  // Verifico se la cartella è stata aperta correttamente
  if (directory == NULL)
  {
    printf("Impossibile aprire la cartella.\n");
    exit(1);
  }
  printf("CIAO");
  fflush(stdout);
  while (stay)
  {
    if (lt_ack_rcvd == seqnum && stay == true)
    {
      // Lettura dei file all'interno della cartella
      while ((file = readdir(directory)) != NULL && swnd < CongWin && stay == true && swnd < lt_rwnd)
      {
        // Ignora le voci "." e ".."
        if (strcmp(file->d_name, ".") != 0 && strcmp(file->d_name, "..") != 0)
        {
          // memorizzo i nomi del pkt.pl finchè non raggiungono la MAXLINE
          temp = temp + strlen(file->d_name);
          sprintf(pkt.pl + strlen(pkt.pl), "%s\n", file->d_name);
          printf("%s\n\n", pkt.pl);
          fflush(stdout);
          pkt.pl[temp] = '\0'; // risolvo il problema di scrivere sul buff pieno
          if (strlen(pkt.pl) == MAXLINE)
          { // se ho il pl pieno invio il pkt
            printf("%s\n\n", pkt.pl);
            fflush(stdout);
            swnd++;
            seqnum++;
            pkt.ack = seqnum;
            pkt.finbit = 0;
            retr[i] = pkt;
            i++;
            i = i % dim;
            prob = (double)rand() / RAND_MAX;
            if (prob < p)
            {
              msgPerso++;
              msgTot++;
            }
            else
            {
              if ((sendto(sockfd, &pkt, sizeof(pkt), 0, (struct sockaddr *)&addr, addrlen)) < 0)
              {
                perror("errore in sendto");
                exit(1);
              }
              msgInviati++;
              msgTot++;
            }
          }
        }
        // mando l'ultimo pkt
        if (strlen(pkt.pl) < MAXLINE)
        {
          while (lt_ack_rcvd != seqnum)
          {
            // Aspetto che il thread legga last ack
            usleep(5);
          }
          swnd++;
          // Invio l'ultimo pkt chiudo la connessione
          stay = false;
          pkt.finbit = 1;
          seqnum++;
          pkt.ack = seqnum;
          pkt.pl[temp] = '\0';
          retr[i] = pkt;
          i++;
          i = i % dim;
          prob = (double)rand() / RAND_MAX;
          printf("Server : Chiusura connessione\n");
          if (prob < p)
          {
            msgPerso++;
            msgTot++;
          }
          else
          {
            if ((sendto(sockfd, &pkt, sizeof(pkt), 0, (struct sockaddr *)&addr, addrlen)) < 0)
            {
              perror("errore in sendto");
              exit(1);
            }
            msgInviati++;
            msgTot++;
          }
        }
      }
    }
  }
  // aspetto terminazione del thread
  if (pthread_join(thread_id, NULL) != 0)
  {
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
void send_control(int sockfd, int my_number)
{
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
  tv.tv_usec = 0; // ms waitingee
  tv.tv_sec = 20; // s waiting
                  // dopo 5 s per il server il client si è disconesso ritorno disponibile
  n = select(sizeof(fds) * 8, &fds, NULL, NULL, &tv);
  if (n == 0)
  {
    printf("Server : Client non risponde\n");
    fflush(stdout);
  }
  else if (n == -1)
  {
    perror("Error send_control select");
    exit(1);
  }
  else if (n != 0 && n != -1)
  {
    if ((recvfrom(sockfd, &pkt, sizeof(pkt), 0, (struct sockaddr *)&addr,
                  &addrlen)) < 0)
    {
      perror("errore in recvfrom");
      exit(-1);
    }
    while (pkt.pl[i] != ' ')
    {
      i++;
    }
    strncpy(cd, pkt.pl, i);
    // +1 perchè non voglio lo spazio
    strcpy(name, pkt.pl + i + 1);

    printf(" Server : Msg ricevuto %s\n", pkt.pl);
    fflush(stdout);
    /* ---------------------- gestisco caso get * ----------------------------------------------*/

    if (!strcmp("get", cd))
    {
      bool found = false;
      DIR *dir;
      struct dirent *entry;
      struct stat fileStat;
      // Apri la cartella
      dir = opendir("Server_Files");
      if (dir == NULL)
      {
        perror("Impossibile aprire la cartella");
        exit(1);
      }
      // Leggi i nomi dei file nella cartella
      while ((entry = readdir(dir)) != NULL && !found)
      {
        char fullPath[500]; // Percorso completo del file
        snprintf(fullPath, sizeof(fullPath), "%s/%s", "Server_Files",
                 entry->d_name);
        if (stat(fullPath, &fileStat) == 0 && S_ISREG(fileStat.st_mode) &&
            !strcmp(entry->d_name, name))
        {
          found = true;
        }
      }
      if (found)
      {
        pkt.ack = 0;
        pkt.finbit = 0;
        strcpy(pkt.pl, "File trovato.");
        printf("\nServer : %s\n", pkt.pl);
        if ((sendto(sockfd, &pkt, sizeof(pkt), 0, (struct sockaddr *)&addr,
                    addrlen)) < 0)
        {
          perror("errore in sendto");
          exit(1);
        }
        // entro nella funzione che implementa la send del file
        send_get(name, sockfd);
      }
      else if (!found)
      {
        // Gestisco caso file non trovato
        strcpy(pkt.pl, "File non trovato, riprova.");
        pkt.ack = -1;
        pkt.finbit = 0;
        printf("invio ack con errore[%s]", pkt.pl);
        fflush(stdout);
        if ((sendto(sockfd, &pkt, sizeof(pkt), 0, (struct sockaddr *)&addr,
                    addrlen)) < 0)
        {
          perror("errore in sendto");
          exit(1);
        }
      }
      // Chiudi la cartella
      closedir(dir);
      // continue;
    }

    /* ---------------------------------- gestisco caso * list------------------------------------------------*/

    if (!strcmp("list", cd))
    {
      strcpy(pkt.pl, "List in esecuzione \n");
      pkt.ack = 0;
      pkt.finbit = 0;
      if ((sendto(sockfd, &pkt, sizeof(pkt), 0, (struct sockaddr *)&addr,
                  addrlen)) < 0)
      {
        perror("errore in sendto");
        exit(1);
      }
      send_list(sockfd);
    }

    /* ------------------------ gestisco caso put * --------------------------------------*/
    if (!strcmp("put", cd))
    {
      printf("\nStart command put\n");
      strcpy(pkt.pl, "Put  in esecuzione");
      pkt.ack = 0;
      pkt.finbit = 0;
      if ((sendto(sockfd, &pkt, sizeof(pkt), 0, (struct sockaddr *)&addr,
                  addrlen)) < 0)
      {
        perror("errore in sendto");
        exit(1);
      }
      rcv_put(name, sockfd);
    }
  }
  stop[my_number - 1] = false;
  printf("\nWait for next request my_number ind %d \n", my_number - 1);
  fflush(stdout);
}

void child_main(int k)
{
  printf("child %ld starting my number is %d\n", (long)getpid(),k);
  int listenfd;
  int n_port = SERV_PORT + k;
  if ((listenfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
  { /* crea il socket */
    perror("errore in socket");
    exit(1);
  }
  fd=listenfd;
  memset((void *)&addr, 0, sizeof(addr));
  addr.sin_family = AF_INET;
  addr.sin_addr.s_addr =
      htonl(INADDR_ANY);         /* il server accetta pacchetti su una qualunque delle
                                    sue interfacce di rete */
  addr.sin_port = htons(n_port); /* numero di porta data dal server la uso per accettare una richiesta

  /* assegna l'indirizzo al socket */
  if (bind(listenfd, (struct sockaddr *)&addr, sizeof(addr)) < 0)
  {
    perror("errore in bind");
    exit(1);
  }
  printf("\n thread starting with port %d \n", n_port);
  fflush(stdout);

  for (;;)
  {
    if (stop[k - 1])
    {
      send_control(listenfd, k); /* processa la richiesta */
    }
  }
}
// creo i processi figli
pid_t child_make(int i) {
  pid_t pid;
  if ((pid = fork()) > 0)
    return (pid);                    // processo padre
                                     //
  child_main(i);  // non ritorna mai
}
// blocco i segnali esterni tranne alcuni che decido ctrl
Sigfunc *signal(int signum, Sigfunc *func)
{
  struct sigaction act, oact;
  /* la struttura sigaction memorizza informazioni riguardanti la
  manipolazione del segnale */

  act.sa_handler = func;
  sigemptyset(&act.sa_mask); /* non occorre bloccare nessun altro segnale */
  act.sa_flags = 0;
  if (signum != SIGALRM)
    act.sa_flags |= SA_RESTART;
  if (sigaction(signum, &act, &oact) < 0)
    return (SIG_ERR);
  return (oact.sa_handler);
}
// termina i figli non ho stato di zombie
void sig_int(int signo)
{
  int i;
  /* terminate all thread*/
  for (i = 0; i < nchildren; i++)
  {
    // kill all thread
    syscall(SYS_exit_group, 0);
    if (pthread_join(pids[i], NULL) != 0)
    {
      perror("Error pthread_join");
      exit(1);
    }
  }
}
void sig_time(int signo)
{
  struct st_pkt pkt;
  int k=0;
  if (CongWin > 1)
  {
    CongWin = CongWin / 2;
    swnd = 0; 
  }
  k=lt_ack_rcvd;
  //  implemento la ritrasmissione di tutti i pkt dopo lt_ack_rcvd
  while (k < seqnum)
  {
    //printf("k value %d seqnum value %d swnd value %d Congwin value %d \n", k, seqnum, swnd, CongWin);
    //fflush(stdout);
    while (swnd < CongWin && swnd < lt_rwnd)
    {
      if ((sendto(fd, &retr[k], sizeof(pkt), 0, (struct sockaddr *)&addr, addrlen)) < 0)
      {
        perror("errore in sendto");
        exit(1);
      }
      //printf("\n\n RCV_CONG :: congwin %d, swnd: %d k: %d retr ack :%d lt_rwnd: %d seqnum : %d\n\n", CongWin, swnd, k, retr[k].ack, lt_rwnd, seqnum);
      //fflush(stdout);
      msgRitr++;
      k++;
      swnd++;
    }
    // faccio una rcv
    if (recvfrom(fd, &pkt, sizeof(pkt), 0, (struct sockaddr *)&addr,
                 &addrlen) < 0)
    {
      perror("errore in recvfrom");
      exit(1);
    }
    lt_rwnd = pkt.rwnd;
    if (pkt.ack > lt_ack_rcvd)
    {
      CongWin++;
      lt_ack_rcvd = pkt.ack;
      swnd = seqnum - lt_ack_rcvd;
    }
    // se il client sta scrivendo rallento sennò riempo i buff della socket
    if (!strcmp(pkt.pl, "slow"))
    {
      printf("\nSLOW RCVD\n");
      fflush(stdout);
      bytes_psecond = 100;
      if (setsockopt(fd, SOL_SOCKET, SO_MAX_PACING_RATE,
                     &bytes_psecond, sizeof(bytes_psecond)) < 0)
      {
        perror("Error setsockopt");
        exit(1);
      }
    }
    else
    {
      bytes_psecond = 1000;
      if (setsockopt(fd, SOL_SOCKET, SO_MAX_PACING_RATE,
                     &bytes_psecond, sizeof(bytes_psecond)) < 0)
      {
        perror("Error setsockopt");
        exit(1);
      }
    }
    printf("\n PKT  RCV : finbit = %d ack %d \n", pkt.finbit, pkt.ack);
    fflush(stdout);
    if (pkt.finbit == 1 && pkt.ack == seqnum)
    {
      printf("Server : Client disconesso  correttamente \n");
      fflush(stdout);
      return ;
    }
  }
}
int main(int argc, char **argv)
{
  if (argc != 6)
  {
    fprintf(stderr, " utilizzo:<pkt loss probab P , TO[s] , TO[ms] , "
                    "SERV_Port, Inserire numero dei child> \n");
    exit(1);
  }
  // variabili richieste dalla traccia
  p = strtod(argv[1], NULL);
  TOs = atoi(argv[2]);
  TOms = atoi(argv[3]);
  SERV_PORT = atoi(argv[4]);
  nchildren = atoi(argv[5]);
  printf("\nProb inserita %f\n", p);
  printf("\nTO inserito %d [s] \n", TOs);
  printf("\nTO inserito %d [ms] \n", TOms);
  printf("\nSERV_PORT inserito %d\n", SERV_PORT);
  printf("\nNumero thread %d\n ", nchildren);
  // creo un array di booleani per tenere traccia se il processo figlio è in ascolto nella socket o è occupato
  int u = 0, n = 0;
  while (nchildren < 1 || nchildren > 25)
  {
    u++;
    char temp[100];
    printf("Numero thread deve essere >= 1 o <=25");
    fscanf(stdin, "%s", temp);
    nchildren = atoi(temp);
    if (u > 5)
    {
      exit(1);
    }
  }
  stop = malloc(sizeof(bool) * nchildren);
  u = 0;
  if (SERV_PORT < 255)
  {
    char temp[100];
    fprintf(stderr, "inserire num. porta dedicato inserire un numero > 255");
    fscanf(stdin, "%s", temp);
    SERV_PORT = atoi(temp);
    if (u > 5)
    {
      exit(1);
    }
  }
  // variabili
  int listenfd;
  struct st_pkt pkt;
  void sig_int(int);

  if ((listenfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
  { /* crea il socket */
    perror("errore in socket");
    exit(1);
  }
  memset((void *)&addr, 0, sizeof(addr));
  addr.sin_family = AF_INET;
  addr.sin_addr.s_addr =
      htonl(INADDR_ANY);            /* il server accetta pacchetti su una qualunque delle
                                       sue interfacce di rete */
  addr.sin_port = htons(SERV_PORT); /* numero di porta del server la uso per
  accettare una richiesta

  /* assegna l'indirizzo al socket */
  if (bind(listenfd, (struct sockaddr *)&addr, sizeof(addr)) < 0)
  {
    perror("errore in bind");
    exit(1);
  }
  if (signal(SIGINT, sig_int) == SIG_ERR)
  {
    fprintf(stderr, "errore in signal INT ");
    exit(1);
  }
  if (signal(SIGALRM, sig_time) == SIG_ERR)
  {
    fprintf(stderr, "errore in signal INT ");
    exit(1);
  }
  // implemento la logica della prefork con file locking
// qui alloco un area di memoria per mantenere i pid dei child
  pids = (pid_t *)calloc(nchildren, sizeof(pid_t));
  if (pids == NULL) {
    fprintf(stderr, "errore in calloc");
    exit(1);
  }
  int tep = 0;
  for (i = 0; i < nchildren; i++) {
    // inserisco nell array pids i pid dei figli che mi ritornano dalla
    // child_make
    tep = i + 1;  //numero di porta che assegno ai child 
    stop[i] = false;
    pids[i] = child_make(tep); /* parent returns */
  }
  system("mkdir Server_Files");
  while (1)
  {
    if ((recvfrom(listenfd, &pkt, sizeof(pkt), 0, (struct sockaddr *)&addr, &addrlen)) < 0)
    {
      perror("errore in recvfrom");
      exit(1);
    }
    printf("main rcvd pkt finbit %d \n", pkt.finbit);
    if (pkt.finbit == 2)
    {
      n = 0;
      for (int i = 0; i < nchildren; i++)
      {
        printf(" i = %d value bool = %d\n", i, stop[i]);
        fflush(stdout);
        if (!stop[i])
        {
          n = 1;
          pkt.ack = SERV_PORT + i + 1;
          sprintf(pkt.pl, "go");
          pkt.pl[2] = '\0';
          stop[i] = true;
          printf("Server: invio pkt di go port number %d \n ", pkt.ack);
          if ((sendto(listenfd, &pkt, sizeof(pkt), 0, (struct sockaddr *)&addr,
                      addrlen)) < 0)
          {
            perror("errore in sendto");
            exit(1);
          }
        }
      }
    }
    else
    {
      printf("\nerrore di connessione\n");
      exit(1);
    }
    // se tutti i processi sono occupati invio un msg di wait al client ritenterà in futuro
    if (n == 0)
    {
      printf("invio wait \n");
      fflush(stdout);
      sprintf(pkt.pl, "wait");
      pkt.pl[4] = '\0';
      if ((sendto(listenfd, &pkt, sizeof(pkt), 0, (struct sockaddr *)&addr,
                  addrlen)) < 0)
      {
        perror("errore in sendto");
        exit(1);
      }
    }
  }
}