/// @file server.c
/// @brief Contiene l'implementazione del SERVER.

#include "defines.h"

#define TICK "[\033[1;32m✓\033[0m]"

// Tempo di attesa post movimento
#define SLEEP_TIME 2

////////////////////////////////////////////////
//       			VARIABILI "GLOBALI"     			  //
////////////////////////////////////////////////

// DEVICES
// - - - - - - - - - - - - - - - 

// Contiene l'ID della memoria condivisa contente l'array di pid dei device
int shm_pidArrayID = 0;

// Contiene il pid dei 5 devices
pid_t * child_pid; 


// ACK MANAGER
// - - - - - - - - - - - - - - - 

// Contiene il pid dell'ack manager
pid_t ack_manager; 


// ACK LIST
// - - - - - - - - - - - - - - - 
 
// Contiene l'ID della memoria condivisa contente l'ack list
int shm_ackmsgID = 0;

// Puntatore alla ack_list
Acknowledgement * ack_list; 



// SEMAFORI
// - - - - - - - - - - - - - - - 

// Contiene l'ID dell'insieme di semafori
int semID = 0; 



// BOARD
// - - - - - - - - - - - - - - - 

// Contiene la chiave di accesso della BOARD
int shm_boardID = 0; 

// Puntatore alla struttura board
board_t * board; 



//// DO I REALLY NEED THIS SHIT?
// MESSAGE QUEUE
// - - - - - - - - - - - - - - - 

// Contiene l'ID memoria condivisa MESSAGE QUEUE
int * msgqueueID; 

// Contiene la chiave della message queue
int msgqueueKEY; 

// Puntatore all'array contente il path del file posizioni
char * path2file; 


////////////////////////////////////////////////
//       	 PROTOTIPI FUNZIONI	         			  //
////////////////////////////////////////////////

void printBoard(board_t * board);

void printPosition();

void clearBoad(board_t * board);

int cleanFolder();

typedef void (*sighandler_t)(int);

void sigHandler(int sig);

void close_all();

char * pathBuilder(pid_t pid_receiver);

int ack_list_checker(int message_id_array, int * position2free);

void clean_ack_list(int * position2free);

int check_receive_acklist(int tmp_messageID, pid_t child_pid);

int first_last_sender_check(int tmp_messageID, int pid_sender);

////////////////////////////////////////////////
//     				  	 MAIN				         			  //
////////////////////////////////////////////////

int main(int argc, char * argv[]) {

  // Controllo input da linea di comando
  if (argc != 3) {
    printf("Usage: %s msq_key | %s miss file_posizioni.txt path\n", argv[0], argv[1]);
    exit(1);
  }

  // - - - - - - - - - - - - - - - 
  //      			VARIABILI
  // - - - - - - - - - - - - - - - 


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

  // Contiene la posizione degli ack da togliere (vedi -> ack_list)
  int position2free[5];


  // - - - - - - - - - - - - - - - 
  //     	INIZIALIZZAZIONE
  // - - - - - - - - - - - - - - - 

	printf("\n\033[1;36m\t\t\t\t\tGreetings, I'm Father.\n\t\t( ・_・)ノ\tShould you ever be in need of\n\t\t\t\t\tkilling me, my pid is \033[0m%d\033[1;36m.\033[0m\n\n\n",getpid());

  printf("\t\t\t\t\033[1;32m-- Inizializations -- \033[0m\n\n");



  // FIFO
  // - - - - - - - - - - - - - - - 

  // Cancellazione file all'interno della cartella fifo
  if(cleanFolder("./fifo")!=0)
    printf("[\033[1;31mx\033[0m] WARNING: Fifo folder not cleaned!\n");
    
    
  open_filePosition(path2file);
  // Appertura file posizioni indicato dal path inserito a terminale
  printf("%s File input \"file_posizioni.txt\"loaded\n\n",TICK);


	// Pulisci cartella output, per comodità nostra
  if (cleanFolder("./output") != 0)
    printf("[\033[1;31mx\033[0m] WARNING: Output folder not cleaned (this is ok if output folder is nonexistant)\n");

  //Stampa delle posizioni lette:
  //printPosition();


  // SEMAFORI
  // - - - - - - - - - - - - - - - 

  // Creazione del set di semafori
  semID = semget(IPC_PRIVATE, 7, S_IRUSR | S_IWUSR);
  if (semID == -1)
    errExit("<Server> Semaphore creation failed!\n");
  printf("%s Semaphore set: created ",TICK);

	// Array contente il valore iniziale dei semafori
	unsigned short semInitVal[] = {
    0, // 0 - dev 0
    0, // 1 - dev 1
    0, // 2 - dev 2
    0, // 3 - dev 3
    0, // 4 - dev 4
    0, // 5 - board
    1  // 6 - acklist
  };

  union semun arg;

  arg.array = semInitVal;

  // Inizializzazione set semafori
  if (semctl(semID, 0, SETALL, arg) == -1)
    errExit("<Server> Semaphore set initialization failed\n");
  printf("and initialized\n\n");


  // SHARED MEMORY
  // - - - - - - - - - - - - - - - 

  // - - BOARD - -

  // Crea il segmento di memoria condivisa da qualche parte nella memoria.
  shm_boardID = alloc_shared_memory(IPC_PRIVATE, sizeof(board_t));

  // Attachnment segmento shared memory della board
  board = (board_t * ) get_shared_memory(shm_boardID, 0);

  printf("%s Board: shared memory allocated and initialized\n\n",TICK);

  // - - ACKLIST - -

  // Crea il segmento di memoria condivisa da qualche parte nella memoria.
  shm_ackmsgID = alloc_shared_memory(IPC_PRIVATE, ACK_LIST_SIZE * sizeof(Acknowledgement));

  // Attachnment segmento shared memory della board
  ack_list = (Acknowledgement * ) get_shared_memory(shm_ackmsgID, 0);

  printf("%s Acklist: shared memory allocated and initialized\n\n",TICK);

  // - - ARRAY PID - -
  // ! ABBIAMO BISOGNO CHE SIA IN MEMORIA CONDIVISA, PERCHÉ ALTRIMENTI I CHILDREN NON VI ACCEDONO (ovvero, anche dichiarandolo globale, ognuno vede solo la sua copia, la quale è riempita solo per i child a lui precedenti)
  // Crea il segmento di memoria condivisa da qualche parte nella memoria.
  shm_pidArrayID = alloc_shared_memory(IPC_PRIVATE, 5 * sizeof(pid_t));

  // Attachnment segmento shared memory della board
  child_pid = (pid_t * ) get_shared_memory(shm_pidArrayID, 0);

  printf("%s Pid array: shared memory allocated and initialized\n\n",TICK);


	//SHIT
	// - - MSG QUEUE - - 

	// Crea il segmento di memoria condivisa da qualche parte nella memoria.
  msgqueueKEY = alloc_shared_memory(IPC_PRIVATE, sizeof(int));

  // Attachnment segmento shared memory della board
  msgqueueID = (int *) get_shared_memory(msgqueueKEY, 0);

  printf("%s Message queue ID: shared memory allocated and initialized\n\n",TICK);


  // SIGHANDLER
  // - - - - - - - - - - - - - - - 

	// Set contenente un insieme di segnali
	sigset_t mySet;

	// Inizializziamo mySet affinché contenga tutti i segnali eccetto SIGTERM
	sigfillset(&mySet);
	sigdelset(&mySet, SIGTERM);
	
	/*
	sigdelset(&mySet, SIGINT);
	sigprocmask(SIG_SETMASK, &mySet, NULL);
	*/
	
	if(signal(SIGTERM, sigHandler) == SIG_ERR)
		errExit("Failed to set sigHandler");

	/*
	if(signal(SIGINT, sigHandler) == SIG_ERR)
		errExit("Failed to set sigHandler");
	*/
	
  // - - - - - - - - - - - - - - - - - 
  // INIZIO ESECUZIONE VERA E PROPRIA 
  // - - - - - - - - - - - - - - - - -

  printf("\n\n\t\t\t\t\t\033[1;32m-- Operations -- \033[0m\n");

  // - - - - - - - - - - - - - - - 
  // CREAZIONE E CODICE DEVICES 
  // - - - - - - - - - - - - - - - 
  for (int child = 0; child < 5; child++) {

    pid_t pid = fork();

    if (pid == -1)
      errExit("\n<Server> Devices creation failed");

		// CODICE I-ESIMO FIGLIO
		// - - - - - - - - - - - - - - - 
    if (pid == 0) {

			// VARIABILI DEL DEVICE
			// - - - - - - - - - - - - - - - 

      // Contiene il pid dei device nelle rispettive posizioni
      child_pid[child] = getpid();

      // Mantiene traccia dell'ultima cella utilizzata dell'array inbox_t
      int lastposition = 0;
			
			// Array contente il path per creare le fifo
			char path2fifo[100];

			// Mantiene traccia dei cicli eseguiti dal sistema
      int step = 0;

			// RESET SIGHANDLER
		  // - - - - - - - - - - - - - - - 

			//COMMENTARE:
			if(signal(SIGINT, SIG_DFL) ==SIG_ERR)
			errExit("Couldn't reset SIGINT sighandler!!");
			if(signal(SIGTERM, SIG_DFL) ==SIG_ERR)
			errExit("Couldn't reset SIGTERM sighandler!!");

			// FIFO
			// - - - - - - - - - - - - - - - 

      // Formulazione del path di ogni fifo legata al pid di ogni processo device.
      sprintf(path2fifo, "./fifo/dev_fifo.%d\n", getpid());

      // Creazione fifo (col nome appropriato appena trovato.)
      if (mkfifo(path2fifo, S_IRUSR | S_IWUSR) == -1)
        errExit("A device failed to create the fifo");

      // Appertura della fifo 
      int fd_fifo = open(path2fifo, O_RDONLY | O_NONBLOCK);
      if (fd_fifo == -1)
        errExit("A device Could not open fifo");


			// MESSAGGI, INBOX, LETTURA, CREAZIONE E GESTIONE ACK, MOVIMENTO
			// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
      while (positionMatrix[step][0] != 999 && step < LIMITE_POSIZIONI) {
				
				// - - - - - - - - - - - - - - - - - - //
        // QUI E'	ZONA MUTUALMENTE ESCLUSIVA!! //
				// - - - - - - - - - - - - - - - - - - //

				// Chiusura i-esimo semaforo
        semOp(semID, child, -1);

				// VARIABILI
				// - - - - - - - - - - - - - - - 

        // Contenitore di un singolo messaggio
        message msg;

        // Utile alle read
        ssize_t bR;

        // Inbox dei messaggi (defines.h -> inbox_t)
        inbox_t inbox[100];


        // RICEZIONE MESSAGGI, INBOX e ACK
        // - - - - - - - - - - - - - - - 
        do {
					/*
					Lettura della fifo in ricerca di messaggi e check
					*/
          bR = read(fd_fifo, & msg, sizeof(msg));
          if (bR == -1)
            errExit("The device couldn't read its fifo :(");

          if (bR != 0) {
						//Notifica: nuovo messaggio
            printf("\033[0;93m[+] <D%d> I received message %d ", child,msg.message_id);

            // Scrittura del messaggio in inbox[lastposition]
            inbox_t msgarrivo = {
              .msg = msg,
              .firstSent = 0
            };

            inbox[lastposition] = msgarrivo;

            lastposition++;

            // ACK e ACK LIST
						// - - - - - - - - - - - - - - - 

						// Creazione ack
            Acknowledgement ack = {
              .pid_sender = msg.pid_sender,
              .pid_receiver = getpid(),
              .message_id = msg.message_id,
              .timestamp = time(NULL)
            };

            //DEBUG: stampa della board di debug
            //printBoard(board);

            int z = 0;

            // Chiusura semaforo dell'ack list
            semOp(semID, 6, -1);

            // Ciclo utile a scorrere l'ack_list finchè non trova cella libera (message_id == 0)
            while (ack_list[z].message_id != 0) {
              if (z == ACK_LIST_SIZE)
                errExit("The ack_list is full.");
              z++;
            }

            // Inserimento ack in ack_list
            ack_list[z] = ack;

            //Stampa del messaggio di conferma ack
            printf("and wrote its ack.\033[0m\n");
					

          }
        } while (bR != 0);

        semOp(semID, 6, 1);

        // SCRITTURA MESSAGGI
        // - - - - - - - - - - - - - - - 

        //						abbiamo un messaggio? 	Ho ancora questo messaggio?
        //												|									|
        //												v									|
        for (int i = 0; i < lastposition; i++) { // V
          for (int receiver = 0; receiver < 5 && inbox[i].firstSent == 0; receiver++) {

						// Calcolo distanza euclidea tra due device
						double dist = distanceCalculator(
              positionMatrix[step][2 * child], // x 1
              positionMatrix[step][2 * child + 1], // y 1
              positionMatrix[step][2 * receiver], // x 2
              positionMatrix[step][2 * receiver + 1]); // y 2

						// Chiusura semaforo ack list
						semOp(semID, 6, -1);

						// Contiene temporaneamente l'ID del messaggio da confrontare
						int tmp_messageID = inbox[i].msg.message_id;

						/*
						Controlla 
						Se 1: il messaggio può essere spedito
						Se 0: il messaggio è già stato ricevuto
						*/
						int flag1 = check_receive_acklist(tmp_messageID, child_pid[receiver]);

						/*
						Controlla 
						Se 1: il messaggio può essere spedito
						Se 0: il messaggio è già stato ricevuto
						*/
						int flag2 = first_last_sender_check(tmp_messageID, inbox[i].msg.pid_sender);

						// Se flag2 == 0 il device è l'ultimo dei 5 ad aver ricevuto il messaggio -> deve toglierlo dalla priopria coda.
						if(flag2 == 0){
							//DEBUG: stampa conferma cancellazione ultimo messaggio da device
							printf("\033[0;93m         I was the last one to receive message %d!\n         I removed it from my inbox.\033[0m\n",msg.message_id);
							inbox[i].firstSent = 1;
						}

						/* 
						Se accede al codice dell'if inserisce l'ack nella lista. Condizioni: 
						 1) distanza appropriata 
						 2) non primo step 
						 3) receiver != sender 
						 4) flag di ricezione da acklist
						*/
            if (dist <= inbox[i].msg.max_distance && step != 0 && receiver != child && flag1 == 1 && flag2 == 1) {	
              // Invio del messaggio via fifo
              pid_t pid_receiver = child_pid[receiver];

							// Formulazione path per fifo ricevente
              char path2fifoWR[100];
              sprintf(path2fifoWR, "./fifo/dev_fifo.%d\n", pid_receiver);

              // Appertura con check della fifo del ricevente
              int fd_write = open(path2fifoWR, O_WRONLY);
              if (fd_write == -1)
                errExit("Device can't open the receiver device fifo");

              inbox[i].msg.pid_sender = getpid();

              // Scrittura con check del messaggio nella fifo del ricevente 
              if (write(fd_write, & inbox[i].msg, sizeof(inbox[i].msg)) == -1)
                errExit("Device couldn't send message to another device");

              // Chiusura del file descriptor
              close(fd_write);

							// Flag di segnalazione di invio
              inbox[i].firstSent = 1;

              //DEBUG: stampa di conferma invio
              printf("\033[0;93m[-] <D%d> I sent message %d to D%d\n\033[0m", child, msg.message_id, receiver);
            }
						
						// Appertura semaforo ack list
						semOp(semID, 6, 1);
          }
        }

        // STAMPE VARIE
        // - - - - - - - - - - - - - - - 

				// Stampa dei messaggi ancora non inviati di un device
        printf("<D%d -> %d> (%d,%d) | msgs: ", child, getpid(), positionMatrix[step][2 * child], positionMatrix[step][2 * child + 1]);

        if (lastposition == 0)
          printf("\033[0;34m[empty]\033[0m");
        else {
					int hasPrinted=0;
          for (int i = 0; i < lastposition; i++) {
            if (inbox[i].firstSent == 0){
              printf("(id: %d)", inbox[i].msg.message_id);
							hasPrinted=1;
							}
          }
					if(hasPrinted==0)
          printf("\033[0;34m[\"empty\"]\033[0m");
        }

        printf("\n");
        fflush(stdout);

        // MOVIMENTO
        // - - - - - - - - - - - - - - - 

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
          semOp(semID, 5, 1); // riapre l'accesso alla board x il server
        }
      }

      exit(0);
    }
  }


  // - - - - - - - - - - - - - - - 
  // CODICE ACK MANAGER
  // - - - - - - - - - - - - - - - 

  // Fork con check del processo
  ack_manager = fork();
  if (ack_manager == -1)
    errExit("Ack manager fork() failed.");

	//Codice ack manager

  if (ack_manager == 0) {

		// RESET SIGHANDLER
		// - - - - - - - - - - - - - - - 

		//COMMENTARE:
		if(signal(SIGINT, SIG_DFL) ==SIG_ERR)
		errExit("Couldn't reset SIGINT sighandler!!");
		if(signal(SIGTERM, SIG_DFL) ==SIG_ERR)
		errExit("Couldn't reset SIGTERM sighandler!!");


		// Creazione con check della msg queue mediante key passata come parametro (server)
		*msgqueueID = msgget(atoi(argv[1]), S_IRUSR| S_IWUSR | IPC_CREAT | IPC_EXCL);
		if(*msgqueueID == -1)
		  errExit("Message queue creation failed! ");
		printf("\n%s Message queue: created. :D\n",TICK);

		// Ciclo con attesa di 5 secondi nel quale vi è il check degli acklist ed eventuale cancellazione
		// - - - - - - - - - - - - - - - 
    while (1) {
      sleep(5);

      // Chiusura del semaforo della ack list
      semOp(semID, 6, -1);

      for (int i = 0; i < ACK_LIST_SIZE; i++) {
		
				// Controlla se ci sono 5 ack con medesimo ID
				if (ack_list[i].message_id != 0 && ack_list_checker(ack_list[i].message_id, position2free)){
          
					// MSG QUEUE e ACK
					// - - - - - - - - - - - - - - - 

					// Preparazione messaggio da inviare
					ack_msgq mex;

					//mtype acquisisce il valore del message id
					mex.mtype = ack_list[position2free[0]].message_id;

					for(int j = 0; j < 5; j++){
						mex.ack_msgq_array[j] = ack_list[position2free[j]];
					//DEBUG:	printf("%d   ",ack_list[position2free[j]].pid_sender);
					}
					
					//printf("\n\033[0;93m<AckManager> I created the ack for message %d.\033[m\n",ack_list[position2free[0]].message_id);

					printf("\n");


					// Invio del messaggio con check sulla msg queue
					if(msgsnd(*msgqueueID, &mex, sizeof(ack_msgq) - sizeof(long), IPC_NOWAIT) == -1)
						errExit("Couldn't read the message queue");

					// PULIZIA ACK_LIST
					// - - - - - - - - - - - - - - - 
					//DEBUG: stamap di successo
					printf("\033[0;93m<AckManager> All devices have received message %d\n             I sent the acks back to the client.\033[m\n",ack_list[position2free[0]].message_id);
		
					// Imposta a 0 il messagge ID dato nella ack_list
					clean_ack_list(position2free);
					

		    }
      }

      // Appertura del semaforo della ack list
      semOp(semID, 6, 1);
    }
  }
  
  // - - - - - - - - - - - - - - - 
	// STAMPE E GESTIONE STEP	
	// - - - - - - - - - - - - - - - 

  clearBoad(board);

  // APERTURA fifo in scrittura (ondevitare blocco dei child finché non arriva il client)

  // Ciclo di spostamento dei device
  int step;

  for (step = 0; positionMatrix[step][0] != 999 && step < LIMITE_POSIZIONI; step++) {

    clearBoad(board);

    printf("\n═══════════════ [✓] Step %d  ════════════════════╗\nStep %d: device positions #####################\n\n", step, step);

    fflush(stdout);

    // apri il semaforo del primo device
    semOp(semID, 0, 1);

    // qui eseguono i figli
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

	// ATTESA CHIUSURA FIGLI E TERMINAZIONE
	// - - - - - - - - - - - - - - - 


  printf("\t\t\t\t\t\t\033[1;32m-- Ending -- \033[0m\n\n");


	printf("Waiting for AckManager to send out any remaining ack...\n");
	fflush(stdout);
	sleep(5);
  // Kill processo ack manager
  kill(ack_manager, SIGTERM);
  printf("\n%s Ack manager: killed\n", TICK);
	
  // Prima di procedere con la chiusura attende la terminazione dei device.
  while (wait(NULL) != -1);

  //DEBUG: qui i figli sono morti :( 
  printf("\n%s All children processes have terminated.\n\n",TICK);


  // Funzione di terminazione, chiude tutto 
  close_all();
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
int cleanFolder(char* folder) {

  // These are data types defined in the "dirent" header
  DIR * cartellaFifo = opendir(folder);

  if (cartellaFifo == NULL)
    return 1;

else{
  struct dirent * next_file;

  char filepath[300];

  while ((next_file = readdir(cartellaFifo)) != NULL) {
    // build the path for each file in the folder
    sprintf(filepath, "%s/%s", folder, next_file -> d_name);

    if (!(strcmp(next_file -> d_name, "..") == 0) &&
      !(strcmp(next_file -> d_name, ".") == 0))
      if (remove(filepath) != 0)
        errExit("Cleaning fifo folder failed!");
  }

  closedir(cartellaFifo);

  printf("%s Folder \"%s\" cleaned\n\n",TICK,folder);

  return 0;
	}
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
	//Detach e delete shared memory MSGID



	msgctl(*msgqueueID, IPC_RMID, NULL);
	free_shared_memory(msgqueueID);
  remove_shared_memory(msgqueueKEY);
  printf("%s Message queue ID shared memory: deattached and removed\n",TICK);

	//Detach e delete shared memory CHILD_PID
  free_shared_memory(child_pid);
  remove_shared_memory(shm_pidArrayID);
  printf("\n%s Pid array shared memory: deattached and removed\n",TICK);

  // Detach  e delete shared memory BOARD
  free_shared_memory(board);
  remove_shared_memory(shm_boardID);
  printf("\n%s Board shared memory: deattached and removed\n",TICK);

  // Detach  e delete shared memory LISTA ACK
  free_shared_memory(ack_list);
  remove_shared_memory(shm_ackmsgID);
  printf("\n%s Acklist shared memory: deattached and removed\n", TICK);

  // Remove SEMAPHORE SET
  if (semctl(semID, 0, IPC_RMID, NULL) == -1)
    errExit("semctl IPC_RMID failed");
  printf("\n%s Semaphore set: deallocated and removed\n\n", TICK);
/*
  if (cleanFolder("./fifo") != 0)
    printf("[\033[1;31mx\033[0m] WARNING:Fifo folder not cleaned");
*/

	printf("\n\n%s THE END! :D %s\n\n",TICK,TICK);
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

/*
Controlla se ci sono 5 messaggi con medesimo id nella ack_list
*/
int ack_list_checker(int message_id_array, int * position2free) {
  int k = 0;
  for (int i = 0; i < ACK_LIST_SIZE; i++) {
    if (ack_list[i].message_id == message_id_array) {
      position2free[k] = i;
      k++;
    }
		//DEBUG: stampa confronto receiver 
		//printf("%d <-> %d\n",mespidrray, ack_list[i].message_id);
  }
  // Se k == 5 vuol dre che ha inserito 5 posizioni, ovvero, ci sono 5 messaggi con id uguale
  if (k == 5)
    return 1;
  else
    return 0;
}

/*
Imposta il message_id dato come parametro a 0 all'interno dell'ack_list
*/
void clean_ack_list(int * position2free) {
  for (int i = 0; i < 5; i++) {
    ack_list[position2free[i]].message_id = 0;
  }
}

/*
Controlla se esiste un ack per un dato messaggio. Esegue il controllo prima dell'invio di un messaggio verso un device ricevente.

Valori di ritorno:
	- 0: se c'è l'ack
	- 1: se non c'è l'ack

*/
int check_receive_acklist(int tmp_messageID, pid_t child_pid){
	int res = 1;
	
	for(int i = 0; i < ACK_LIST_SIZE; i++){
		if(ack_list[i].message_id == tmp_messageID && ack_list[i].pid_receiver == child_pid)
			res = 0;	
	}
	
	return res;
}

/*
Controlla se il device che possiede il messaggio con dato message_id è il primo a ricevere tale messaggio (da un client) oppure se è l'ultimo ad averlo ricevuto.

Il controllo si basa su sender del messaggio (se diverso da pid di device -> client).

Valori di ritorno:
	- 0: se è l'ultimo device ad aver ricevuto il messaggio;
	- 1: se è un device che può inviare il messaggio.
*/
int first_last_sender_check(int tmp_messageID, int pid_sender){
	int numberAck = 0;
	int isFirstLast = 1;
	int tmp;

	// Controlla se ci sono (mediante o non esclusivo) altri ack nella ack_list (un controllo per figlio)
	for(int i = 0; i < 5; i++){
		// Se = 1 indica che il device è il primo o l'ultimo

		tmp = check_receive_acklist(tmp_messageID, child_pid[i]);

		if(tmp == 0)
			numberAck++;

		// Esegue il controllo
		if(child_pid[i] != getpid()){
			isFirstLast = isFirstLast * tmp;
		}
	}

	if(numberAck == 5){
		return 0;
	}

	// Se = 0 indica che il device è di mezzo
	if(isFirstLast == 0)
		return 1; //perché può spedire normalmente.


	// Bisogna controllare se è il primo o l'ultimo
	else{
		// Se è il primo spedisce normalemente -> controllo su pid sender (!= child_pid)
		for(int k = 0; k < 5; k++){
			if(pid_sender == child_pid[k])
				// se flag = 0, allora NON è il primo, ma l'ultimo (altri casi esclusi da for precedente)
				return 0; 
		}
	return 1;
	}	
}

/*
La funzione impedisce a qualunque segnale (eccetto SIGTERM) di terminare il flusso di esecuzione.

Alla ricezione di un SIGTERM eseguirà la funzione di chiusura.
*/
void sigHandler(int sig) {	
	
	printf("\n\n\t\033[1;36m ( ・_・)  It's time to die. Farewell!\033[0m\n\n");
	close_all();
  
	exit(0);
}
