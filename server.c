/// @file server.c
/// @brief Contiene l'implementazione del SERVER.

#include "defines.h"

////////////////////////////////////////////////
//       			VARIABILI "GLOBALI"     			  //
////////////////////////////////////////////////

pid_t * child_pid; // pid dei device

int semID = 0; // id dell'insieme dei semafori utilizzati

int shm_boardId = 0; // Chiave di accesso della board

board_t * board; // Puntatore alla struttura board

int shm_ackmsgID = 0; // Chiave accesso alla memoria condivisa ack

Acknowledgement * ack_list; // Puntatore alla lista ack

char * path2file; //(file posizioni)

////////////////////////////////////////////////
//       	 PROTOTIPI FUNZIONI	         			  //
////////////////////////////////////////////////

void printBoard(board_t * board);

void printPosition();

void clearBoad(board_t * board);

int cleanFifoFolder();

void sigHandler(int sig);

void close_all();

char * pathBuilder(pid_t pid_receiver);

////////////////////////////////////////////////
//     				  	 MAIN				         			  //
////////////////////////////////////////////////

int main(int argc, char * argv[]) {

  // check command line input arguments
  if (argc != 3) {
    printf("Usage: %s msq_key | %s miss file_posizioni.txt path\n", argv[0], argv[1]);
    exit(1);
  }

  ///////////////////////////////////////
  //      			VARIABILI				     	  //
  ///////////////////////////////////////

  // Path contente il file posizioni
  path2file = argv[2];

  // Array contente i file descriptor usati per le open delle fifo
  int fd_fifo[5] = {
    0,
    0,
    0,
    0,
    0
  };

  //Contiene l'ID della memoria condivisa contente l'array di pid dei device
  int shm_pidArrayID = 0;

  ////////////////////////////////////////////////
  //       			INIZIALIZZAZIONE	      		   //
  ////////////////////////////////////////////////

  printf("\n-- Inizializations -- \n");

  ////
  // FIFO
  ////

  // Cancellazione file all'interno della cartella fifo
  cleanFifoFolder();

  open_filePosition(path2file);
  // Appertura file posizioni indicato dal path inserito a terminale
  printf("\n[✓] file_posizioni.txt loaded\n\n");

  //DEBUG: Stampa delle posizioni lette
  //printPosition();

  ////
  // SEMAFORI
  ////

  // Creazione del set di semafori
  semID = semget(IPC_PRIVATE, 7, S_IRUSR | S_IWUSR);
  if (semID == -1)
    errExit("[x] <Server> Semaphore creation failed!\n");
  printf("[✓] Semaphore set: created\n");

  unsigned short semInitVal[] = {
    0, //dev 0
    0, //dev 1
    0, //dev 2
    0, //dev 3
    0, //dev 4
    0, //board
    0 //acklist
  };

  union semun arg;
  arg.array = semInitVal;

  // Inizializzazione 
  if (semctl(semID, 0, SETALL, arg) == -1)
    errExit("[x] <Server> initialization semaphore set failed\n");
  printf("[✓] Semaphore set: initialized\n");

  ////
  // MEMORIA CONDIVISA
  ////

  // BOARD

  // Crea il segmento di memoria condivisa da qualche parte nella memoria.
  shm_boardId = alloc_shared_memory(IPC_PRIVATE, sizeof(board_t));

  // Attachnment segmento shared memory della board
  board = (board_t * ) get_shared_memory(shm_boardId, 0);

  printf("[✓] Board: shared memory allocated and initialized\n");

  //ACKLIST

  // Crea il segmento di memoria condivisa da qualche parte nella memoria.
  shm_ackmsgID = alloc_shared_memory(IPC_PRIVATE, ACK_LIST_SIZE * sizeof(Acknowledgement));

  // Attachnment segmento shared memory della board
  ack_list = (Acknowledgement * ) get_shared_memory(shm_ackmsgID, 0);

  printf("[✓] Acklist: sahred memory allocated and initialized\n\n");

  // ARRAY PID

  // Crea il segmento di memoria condivisa da qualche parte nella memoria.
  shm_pidArrayID = alloc_shared_memory(IPC_PRIVATE, 5 * sizeof(pid_t));

  // Attachnment segmento shared memory della board
  child_pid = (pid_t * ) get_shared_memory(shm_pidArrayID, 0);

  printf("[✓] Pid array: sahred memory allocated and initialized\n\n");

  ////////////////////////////////////////////////
  //   				CODICE PROGRAMMA							   //
  ////////////////////////////////////////////////

  printf("\n\t\t\t  -- Operations -- \n");

  // Ciclo di fork
  for (int child = 0; child < 5; child++) {

    pid_t pid = fork();

    if (pid == -1)
      errExit("\n[x] <Server> Devices creation failed");

    // CODICE ESEGUITO DAL i-ESIMO FIGLIO
    if (pid == 0) {

      // Contiene il pid dei device nelle rispettive posizioni
      child_pid[child] = getpid();

      // Mantiene traccia dell'ultima cella utilizzata dell'array inbox_t
      int lastposition = 0;

      // Formulazione del path di ogni fifo legata al pid di ogni processo device.
      char path2fifo[100];
      sprintf(path2fifo, "./fifo/dev_fifo.%d\n", getpid());

      // Creazione fifo (col nome appropriato appena trovato.)
      if (mkfifo(path2fifo, S_IRUSR | S_IWUSR) == -1)
        errExit("\n[x] A device failed to create the fifo");

      // Gestione del server della lettura e del muovimento 
      int fd_fifo = open(path2fifo, O_RDONLY | O_NONBLOCK);

      if (fd_fifo == -1)
        errExit("[x] Device Could not open fifo :(");

      // Mantiene traccia dei cicli eseguiti dal sistema
      int step = 0;

      while (positionMatrix[step][0] != 999 && step < LIMITE_POSIZIONI) {

        semOp(semID, child, -1);

        ////////////////////////////////////////////////
        // 	QUESTA ZONA è MUTUALMENTE ESCLUSIVA!!!!  //
        ////////////////////////////////////////////////

        // Conteine un messaggio
        message msg;

        // Utile alle read
        ssize_t bR;

        // Inbox dei messaggi (defines.h -> inbox_t)
        inbox_t inbox[100];

        ////
        // MESSAGGI e INBOX
        ////

        do {
          bR = read(fd_fifo, &msg, sizeof(msg));
          if (bR == -1)
            errExit("[x] Device couldn't read fifo :(");

          if (bR != 0) {

            printf("\n[+] **** <D%d> has new message!\n", child);

            // Scrittura del messaggio in inbox[lastposition]
            inbox_t msgarrivo = {
              .msg = msg,
              .firstSent = 0
            };

            inbox[lastposition] = msgarrivo;

            lastposition++;
						
						// Inserimento dell'ack in ack_list
						Acknowledgement ack = {
							.pid_sender = msg.pid_sender,
							.pid_receiver = getpid(),
							.message_id = msg.message_id,
							.timestamp = time(NULL)
						};

						printBoard(board);
						
						// Ciclo utile a scorrere l'ack_list finchè non trova cella libera (message_id == 0)
						int z = 0;
						while(ack_list[z].message_id != 0){
							if(z == ACK_LIST_SIZE)
								errExit("The ack_list is full.");
							z++;
						}

						ack_list[z] = ack;

						//DEBUG: stampa del messaggio di conferma ack
						printf("[✓] Ack was written suceffully.\n\n");
						/* */
          }

				} while (bR != 0);


        ////
        // SCRITTURA MESSAGGI
        ////

        //						abbiamo un messaggio? 	Ho ancora questo messaggio?
        //												|									|
        //												v									|
        for (int i = 0; i < lastposition; i++) { //  V
          for (int receiver = 0; receiver < 5 && inbox[i].firstSent == 0; receiver++) {
            double dist = distanceCalculator(
              positionMatrix[step][2 * child], // x 1
              positionMatrix[step][2 * child + 1], // y 1
              positionMatrix[step][2 * receiver], // x 2
              positionMatrix[step][2 * receiver + 1]); // y 2

            // Se accede al codice dell'if inserisce l'ack nella lista 
            // Condizioni: 1) distanza appropriata 2) non primo step 3) receiver != sender
            if (dist <= inbox[i].msg.max_distance && step != 0 && receiver != child) {

              // Invio del messaggio via fifo
              pid_t pid_receiver = child_pid[receiver];

              char path2fifoWR[100];
              sprintf(path2fifoWR, "./fifo/dev_fifo.%d\n", pid_receiver);

              // Appertura della fifo del ricevente
              int fd_write = open(path2fifoWR, O_WRONLY);
              if (fd_write == -1)
                errExit("[x] device can't open the receiver device fifo");

              // Scrittura del messaggio nella fifo del device ricevente 
              if (write(fd_write, & inbox[i].msg, sizeof(inbox[i].msg)) == -1)
                errExit("[x] Device couldn't send message to another device");

              // Chiusura del file descriptor
              close(fd_write);

              inbox[i].firstSent = 1;

              //DEBUG: stampa di conferma invio
              printf("<D%d> Ho consegnato il messaggio a <D%d>\n", child, receiver);
            }
          }
        }

        ////
        // STAMPA SITUAZIONE
        ////

				printf("<D%d -> %d> (%d,%d) | msgs: ",child,getpid(),positionMatrix[step][2*child],positionMatrix[step][2*child+1]);

				if(lastposition == 0)
					printf("[empty]");
				else{
					for(int i = 0; i < lastposition; i++){
						if(inbox[i].firstSent == 0)
							printf("(id: %d)", inbox[i].msg.message_id);
					}
				}
				
				printf("\n");
				
				fflush(stdout);
				
        ////
        // MOVIMENTO
        ////

        // Calcolo posizione i-esimo device (row, col) [post invio e ricezione]
        int nextrow = positionMatrix[step][2 * child];

        int nextcol = positionMatrix[step][2 * child + 1];
        board -> board[nextrow][nextcol] = getpid();

        // Passaggio allo step successivo
        step++;

        // L'appertura non è circolare.
        // Il server si occupa di settare il primo semaforo a 1
        if (child != 4)
          semOp(semID, child + 1, 1);

        if (child == 4) {
          semOp(semID, 5, 1); //riapri l'accesso alla board x il server
        }
      }

      exit(0);
    }
  }

  ////////////////////////////////////////////////
  //   				GESTIONE DEGLI STEP								//
  ////////////////////////////////////////////////

  clearBoad(board);

  // APERTURA fifo in scrittura (ondevitare blocco dei child finché non arriva il client)

  // Ciclo di spostamento dei device
  int step;

  for (step = 0; positionMatrix[step][0] != 999 && step < LIMITE_POSIZIONI; step++) {

    clearBoad(board);
		
    printf("\n═══════════════ [✓] Step %d  ════════════════════╗\nStep %d: device positions #####################\n\n", step, step);

		fflush(stdout);

    //apri il semaforo del primo device
    semOp(semID, 0, 1);

    //qui eseguono i figli
    semOp(semID, 5, -1);

    printBoard(board);

    printf("#####################\n════════════════════════════════════════════════╝\n\n");

    clearBoad(board);

    //DEBUG: timer (stampato a video) di countdown per ricezione messaggi
    for (int i = 1; i <= SLEEP_TIME; i++) {
      sleep(1);
      printf("%d... ", SLEEP_TIME - i);
      fflush(stdout);
    }

    // Inizio timer (2s) di attesa post-muovimento
    // Sleep di n secondi (delta tempo tra muovimento ultimo device e ripresa operazioni)
    printf("\n");
  }

  // Prima di procedere con la chiusura attende la terminazione dei device.
  while (wait(NULL) != -1);

  //DEBUG: qui i figli sono morti :( 
  printf("\n[✓] Tutti i bambini sono andati a letto.\n");

  ////////////////////////////////////////////////
  //       					CHIUSURA  		    				  //
  ////////////////////////////////////////////////

  printf("\n\n-- Ending --\n\n");

  // Funzione di terminazione, chiude tutto 
  close_all();

  printf("\n\n[✓] THE END! :D\n\n");
}

//////////////////////////////////////////
//       				FUNZIONI        			  //
//////////////////////////////////////////

/*
Stampa le posizioni dei singoli device all'interno della board (la quale è vuota).
*/
void printBoard(board_t * board) {
  char c = ' ';
  pid_t pid;
  printf("\n\t");

  for (int k = 0; k < 10; k++)
    printf("  %d", k);

  printf("\n");

  for (int i = 0; i < 10; i++) {

    printf("\t%d", i);
		
    for (int j = 0; j < 10; j++) {
      pid = board -> board[i][j];
      if (pid == child_pid[0])
        c = '0';
      else if (pid == child_pid[1])
        c = '1';
      else if (pid == child_pid[2])
        c = '2';
      else if (pid == child_pid[3])
        c = '3';
      else if (pid == child_pid[4])
        c = '4';
      else
        c = ' ';
     printf("|%c ", c);
		 //printf("|%d ", board->board[i][j]);
			fflush(stdout);

    }
    printf("|\n");
  }

  printf("\t--------------------------------\n");
}

/*
Stampa le posizioni lette in 'file_posizioni.txt' nella board.
*/
void printPosition() {
  int i = 0, j = 0;

  printf("\t\t\tD1\t\tD2\t\tD3\t\tD4\t\tD5\n");

  for (i = 0; i < LIMITE_POSIZIONI && positionMatrix[i][0] != 999; i++) {

    printf("\tt = %d", i);

    for (j = 0; j < 10; j = j + 2)
      printf("\t%d,%d|", positionMatrix[i][j], positionMatrix[i][j + 1]);
    printf("\n");
  }
  printf("\n");
}

/*
Elimina tutti i file contenti nella cartella './fifo'
*/
int cleanFifoFolder() {

  // These are data types defined in the "dirent" header
  DIR * cartellaFifo = opendir("./fifo");

  if (cartellaFifo == NULL)
    errExit("[x] open directory failed");

  struct dirent * next_file;

  char filepath[300];

  while ((next_file = readdir(cartellaFifo)) != NULL) {
    // build the path for each file in the folder
    sprintf(filepath, "%s/%s", "./fifo", next_file -> d_name);

    if (!(strcmp(next_file -> d_name, "..") == 0) &&
      !(strcmp(next_file -> d_name, ".") == 0))
      if (remove(filepath) != 0)
        errExit("[x] Cleaning fifo folder failed!");
  }

  closedir(cartellaFifo);

  printf("[✓] Fifo folder cleaned\n");

  return 0;
}

/*
Pulisce la board prima di una stampa.
*/
void clearBoad(board_t * board) {
  for (int j = 0; j < 10; j++) {
    for (int i = 0; i < 10; i++)
      board -> board[i][j] = 0;
  }
}

/*
Terminazione di tutte le strutture dati utilizzate.
*/
void close_all() {

  // Detach  e delete shared memory BOARD
  free_shared_memory(board);
  remove_shared_memory(shm_boardId);
  printf("\n[✓] Board: deattached and removed\n");

  // Detach  e delete shared memory LISTA ACK
  free_shared_memory(ack_list);
  remove_shared_memory(shm_ackmsgID);
  printf("\n[✓] Acklist: deattached and removed\n");

  // Remove SEMAPHORE SET
  if (semctl(semID, 0, IPC_RMID, NULL) == -1)
    errExit("[x] semctl IPC_RMID failed");
  printf("\n[✓] Semaphore set: deallocated and removed\n\n");

  if (cleanFifoFolder() != 0)
    errExit("\n[x] Fifo folder not cleaned ./fifo");
}

void sigHandler(int sig) {
  printf("\n\nThe sighandler was started.\n");

  if (sig == SIGTERM) {
    printf("\n\nThe signal SIGTERM was caught.\n");
    close_all();
  }
}

/*
Costruttore del path per le fifo
*/
char * pathBuilder(pid_t pid_receiver) {

  // Formulazione del path di ogni fifo legata al pid di ogni processo device.
  char * path2fifo;

  sprintf(path2fifo, "./fifo/dev_fifo.%d", pid_receiver);

  return path2fifo;
}