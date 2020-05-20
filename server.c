/// @file server.c
/// @brief Contiene l'implementazione del SERVER.
#include "err_exit.h"
#include "defines.h"
#include "shared_memory.h"
#include "semaphore.h"
#include <sys/sem.h>
#include <errno.h>
#include "fifo.h"
#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <time.h>
#include <sys/shm.h>
#include <sys/types.h>
#include <sys/wait.h>

#define ACK_LIST_SIZE 100



pid_t child_pid[5];

// Limite righe lette dal file posizioni
# define LIMITE_POSIZIONI 10

// struttura contenente matrice (-> board)
typedef struct {
  pid_t board[10][10];
}
board_t;

// struttura di un singolo messaggio acknowledge
typedef struct {
  pid_t pid_sender;
  pid_t pid_receiver;
  int message_id;
  time_t timestamp;
}
Acknowledgment;

// Matrice delle coordinate dei devices nel tempo
int positionMatrix[LIMITE_POSIZIONI][10];

//
//
// PROTOTIPO FUNZIONI
//
//

void open_filePosition(char * path2file);
void printBoard(board_t * board);
void printPosition();

//
//
// MAIN 
//
//

int main(int argc, char * argv[]) {

  // check command line input arguments
  if (argc != 3) {
    printf("Usage: %s msq_key | %s percorso file posizioni\n", argv[0], argv[1]);
    exit(1);
  }

///////////////
// VARIABILI //
///////////////

  // Contiene l'id dell'insieme dei semafori utilizzati
  int semID = 0;

  // board
  int shm_boardId = 0; // Chiave di accesso
  board_t * board; // Puntatore a prima cella array (-> prima cella tabella)

  // messaggio e lista acknowledge
  int shm_ackmsgID = 0; // Chiave accesso alla memoria condivisa ack
  Acknowledgment * ack_list; // Puntatore a prima cella array (-> lista acknowledge)


////////////////////////////////////////////////
//       			INIZIALIZZAZIONE	      			  //
////////////////////////////////////////////////

  printf("\n--Inizializzazioni-- \n");
  // Appertura del file indicato nell'argomento inserito da termniale
  char * path2file = argv[2];
  open_filePosition(path2file);
	printPosition();
	printf("[✓] File posizioni caricato\n");


	//Creazione e inizializzazione insieme di SEMAFORI
  semID = semget(IPC_PRIVATE, 7, S_IRUSR | S_IWUSR);
  if (semID == -1)
    errExit("[x] <Server> Semaphore creation failed!");
	printf("[✓] Semafori creati\n");
  unsigned short semInitVal[] = {
   
    1, // <- Il child 1 deve partire per primo, dunque è l'unico sbloccato
		   // Posto a 1 perchè usiamo il metodo "-1 per bloccare"
		0,
    0,
    0,
    0,
    0,
    0
  };

  union semun arg;
  arg.array = semInitVal;
  if (semctl(semID, 0 /*ignored*/ , SETALL, arg) == -1)
    errExit("[x] <Server> initialization semaphore set failed");
	printf("[✓] Semafori inizializzati\n");

  //crea il segmento di memoria condivisa da qualche parte nella memoria.
  shm_boardId = alloc_shared_memory(IPC_PRIVATE, sizeof(board_t));
  // Attachnment segmento shared memory della board
  board = (board_t * ) get_shared_memory(shm_boardId, 0);
	printf("[✓] Memoria board allocata e attaccata\n");	

  //crea il segmento di memoria condivisa da qualche parte nella memoria.
  shm_ackmsgID = alloc_shared_memory(IPC_PRIVATE, ACK_LIST_SIZE * sizeof(Acknowledgment));
  // Attachnment segmento shared memory della board
  ack_list = (Acknowledgment * ) get_shared_memory(shm_boardId, 0);
	printf("[✓] Memoria acklist allocata e attaccata\n");


////////////////////////////////////////////////
//   			Programma vero e proprio					  //
////////////////////////////////////////////////

  printf("\n --Operazioni-- \n");

  for (int child = 0; child < 5; child++) {
		// pid associato a ogni device
    pid_t pid = fork();
		child_pid[child] = pid; //in teoria dovrei avere una copia vuota nei figli e una copia funzionante nel padre. Finché chiamo la roba dal padre tutto ok
    if (pid== -1)
      errExit("[x] <Server> errore creazione device");

		//FIGLIO
     if (pid == 0) {

        //printf("[✓] <D%d> I'm alive! Il mio pid è %d.\n",child, getpid());

        // Formulazione del path di ogni fifo legata al pid di ogni processo device.
				char * fifopathbase = "./fifo/dev_fifo.";
        char path2fifo[100];
        sprintf(path2fifo, "./fifo/dev_fifo.%d\n", getpid());
				// Creazione fifo (col nome appropriato appena trovato.)
				if(mkfifo(path2fifo, S_IRUSR | S_IWUSR)==-1)
					errExit("[x] Un device non è riuscito a creare la fifo");
        //printf("[✓] <D%d> Ho creato la fifo %s", child,path2fifo);
			



			//Sincronizzazione 
        semOp(semID, child, -1);

				////////////////////////////////////////////////
				// 	QUESTA ZONA è MUTUALMENTE ESCLUSIVA!!!!   //
				//printf("[%d] ayy\n\n",child);								//
				////////////////////////////////////////////////

				int nextrow=positionMatrix[0][2*child];
				int nextcol=positionMatrix[0][2*child+1];
        printf("[✓] <D%d %d> --> %d,%d\n",child,getpid(),nextrow,nextcol);
				board->board[nextrow][nextcol]=getpid();

        semOp(semID, (unsigned short)(child == 4) ? 0 : child + 1, 1);
				

        exit(0);
      

    }

  }

	//Prima di procedere con la chiusura attende la terminazione dei device.
  while(wait(NULL) != -1);

	printBoard(board);
	
	printf("\n[✓] Tutti i bambini sono andati a letto.\n");



////////////////////////////////////////////////
//       					CHIUSURA  		    				  //
////////////////////////////////////////////////

	printf("\n\n--Chiusura--\n");

  // Stacca e rimuove shared memory board
  free_shared_memory(board);
  remove_shared_memory(shm_boardId);
	printf("[✓] Board deallocata e rimossa\n");

  //Detach  e delete shared memory LISTA ACK
  free_shared_memory(ack_list);
  remove_shared_memory(shm_ackmsgID);
	printf("[✓] Acklist deallocata e rimossa\n");

	//Rimuove i semafori.
  if (semctl(semID, 0, IPC_RMID, NULL) == -1)
    errExit("[x] semctl IPC_RMID failed");
	printf("[✓] Semafori deallocati e rimossi\n");
	
 
  printf("[✓] Programma terminato! :D\n");
}



//////////////////////////////////////////
//       				Funzioni        			  //
//////////////////////////////////////////

	void open_filePosition(char * path2file) {


		//Apertura file di input.
		int fd = open(path2file, O_RDONLY);
		if (fd == -1)
			errExit("[x] <Device> open file_posizioni.txt fallita");

		// buffer per lettura caratteri (19 caratteri + '\n' + '\0' -> tot. 21)
		char buffer[BUFFER_FILEPOSIZIONI + 1];

		//contatore caratteri letti
		ssize_t bR = 0;


		// ciclo di lettura (legge una riga per volta,
		// e per ciascuna riga )
		int row = 0;
		do {
			bR = read(fd, buffer, BUFFER_FILEPOSIZIONI);
			if (bR == -1)
				errExit("[x] <Device> Lettura da file_posizioni.txt fallita");

			if (bR != 0) {
				int i = 0;
				for (int col = 0; col < (BUFFER_FILEPOSIZIONI); col += 2) {
					positionMatrix[row][i] = buffer[col] - 48;
					i++;
				}
			}

			row++;
		} while (bR != 0 && (row < LIMITE_POSIZIONI)); // iterazione fino a lettura di un valore dal file e row < limite 

		// Valore sentinella 
		//(usato in caso in cui non si raggiunga il limite massimo di righe [LIMITE_POSIZIONI])
		if (row < LIMITE_POSIZIONI - 1) {
			positionMatrix[row - 1][0] = 999;
		}

		// chiusura del file descriptor
		close(fd);
	}
	void printBoard(board_t * board){
		char c= ' ';

	printf("[?] <printBoard> Pid rilevati: ");
		for(int k=0;k<5;k++){
			printf("%d ",child_pid[k]);
		}

	printf("\n\t-----------Board------------\n");
		pid_t pid;
		for(int i=0;i<10;i++){
			printf("\t");
			for(int j=0;j<10;j++){
				pid=board->board[i][j];
				if(pid==child_pid[0])
					c='0';
				else if(pid==child_pid[1])
					c='1';
				else if(pid==child_pid[2])
					c='2';
				else if(pid==child_pid[3])
					c='3';
				else if(pid==child_pid[4])
					c='4';
				else
					c=' ';
				printf("|%c ",c);

				//printf("|%d",board->board[i][j]);
			}
			printf("\n");
		}

			printf("\t----------------------------\n");
	}


	void printPosition(){
		int i=0,j=0;

			printf("\t   |D0 ||D1 ||D2 ||D3 ||D4 |\n");
	
			for(i=0;i<LIMITE_POSIZIONI && positionMatrix[i][j]!=999;i++){
				printf("\tt=%d",i);
				for(j=0;j<10;j=j+2)
					printf("|%d,%d|",positionMatrix[i][j],positionMatrix[i][j+1]);
				printf("\n");
			}


	}