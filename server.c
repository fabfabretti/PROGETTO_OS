/// @file server.c
/// @brief Contiene l'implementazione del SERVER.

#include "err_exit.h"


#include "defines.h"

#include "shared_memory.h"

#include "semaphore.h"
#include <sys/sem.h>

#include "fifo.h"

#include <unistd.h>

#include <fcntl.h>

#include <stdio.h>

#include <time.h>

#include <sys/shm.h>

#include <sys/types.h>


#define ACK_LIST_SIZE 100

// Limite righe lette dal file posizioni
# define LIMITE_POSIZIONI 10

// Lezione fraccaroli 2:06:30
// allocazione shared_memory (board e shared list)
// attach  shared_memory create
// il puntatore alla scacchiera lo da figli. (codice identico a es. share memory/semfory/ecc.)
// prima fork ack_manager poi for loop e fork device

// struttura contenente matrice (-> board)
typedef struct{
  pid_t board[10][10];
} board_t;


// struttura di un singolo messaggio acknowledge
typedef struct {
  pid_t pid_sender;
  pid_t pid_receiver;
  int message_id;
  time_t timestamp;
} Acknowledgment;

// matrice delle posizioni
int positionMatrix[LIMITE_POSIZIONI][10];

//
//
// PROTOTIPO FUNZIONI
//
//

void open_filePosition(char * path2file);

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

  // 
  //
  // VARIABILI
  //
  //

	// semafori
	int semID = 0;


  // board
  int shm_boardId = 0; // Chiave di accesso
  board_t * board; // Puntatore a prima cella array (-> prima cella tabella)

  // messaggio e lista acknowledge
  int shm_ackmsgID = 0; // Chiave accesso alla memoria condivisa ack
  Acknowledgment * ack_list; // Puntatore a prima cella array (-> lista acknowledge)

  // 
  //
  // CODICE
  //
  //

  // generazione proesso ack_manager 
  char * path2file = argv[2];
  open_filePosition(path2file);

	//##############: Creazione e inizializzazione insieme di semafori

	/* Creazione insieme di semafori {scacchiera, D1, D2, D3, D4, D5, listaACK}*/
	printf("\n<Server> Creation semaphore set [...]");
	semID = semget(IPC_PRIVATE, 7, S_IRUSR | S_IWUSR);
	if(semID==-1)
		errExit("<Server> Semaphore creation failed!");

	/*Inizializzazione:*/

	unsigned short semInitVal[] = {0, 0, 0, 0, 0, 0, 0};
	union semun arg;
	arg.array = semInitVal;

	if (semctl(semID, 0 /*ignored*/, SETALL, arg) == -1)
		errExit("<Server> initialization semaphore set failed");

  printf("\n## ## ## ## ## ## ## ##\n");

  //############## Allocazione shared memory per la BOARD
  printf("\n<Server> Allocation shared memory board [...]");
  shm_boardId = alloc_shared_memory(IPC_PRIVATE, sizeof(board_t));
  //crea il segmento di memoria condivisa da qualche parte nella memoria.

  // Attachnment segmento shared memory della board
  printf("\n<Server> Attachement shared memory board [...]\n");
  board = (board_t * ) get_shared_memory(shm_boardId, 0);

  printf("\n## ## ## ## ## ## ## ##\n");

  //############## Allocazione shared memory per la LISTA ACK
  printf("\n<Server> Allocation shared memory list [...]");
  shm_ackmsgID = alloc_shared_memory(IPC_PRIVATE, ACK_LIST_SIZE * sizeof(Acknowledgment));
  //crea il segmento di memoria condivisa da qualche parte nella memoria.

  // Attachnment segmento shared memory della board
  printf("\n<Server> Attachement shared memory list [...]\n");
  ack_list = (
    Acknowledgment * ) get_shared_memory(shm_boardId, 0);

  printf("\n## ## ## ## ## ## ## ##\n");

  //TODO: Fork per i 5 figli
  printf("\n<Server> Creazione 5 device (fork). [...] \n");

  for (int child = 0; child < 5; child++) {

    pid_t pid = fork();

    if (pid == -1)
      errExit("<Server> errore creazione device");

    else {
      if (pid == 0) {

        printf("Sono il figlio %d", child);
        break;
      }
    }

    exit(0); //??
  }

  printf("\n## ## ## ## ## ## ## ##\n");

  //############## Eliminazione insieme dei semafori
	printf("\n<Server> Removing semaphore set [...]");
	if (semctl(semID, 0 , IPC_RMID, NULL) == -1)
        errExit("semctl IPC_RMID failed");

  printf("\n## ## ## ## ## ## ## ##\n");

  //############## Detach shared memory BOARD
  printf("\n<Server> Detaching shared memory board [...]");
  free_shared_memory(board);

  // Deleting shared memory board
  printf("\n<Server> Removing shared memory board [...]\n");
  remove_shared_memory(shm_boardId);

  printf("\n## ## ## ## ## ## ## ##\n");

  //############## Detach shared memory LISTA ACK
  printf("\n<Server> Detaching shared memory lista ack [...]");
  free_shared_memory(ack_list);

  // Deleting shared memory lista ack 
  printf("\n<Server> Removing shared memory lista ack [...]\n");
  remove_shared_memory(shm_ackmsgID);

  printf("\n## ## ## ## ## ## ## ##\n");

  printf("\nProgramma terminato");
}

//
//
// FUNZIONI
//
//

void open_filePosition(char * path2file) {

  int fd = open(path2file, O_RDONLY);
  // verifica errori di appertura
  if (fd == -1)
    errExit("<device> open file_posizioni.txt fallita");

  // buffer per lettura caratteri (19 caratteri + '\n' + '\0' -> tot. 21)
  char buffer[BUFFER_FILEPOSIZIONI + 1];

  ssize_t bR = 0;

  int row = 0;
  do {
    printf("\n GIRO \n");
    // lettura di una riga del file posizioni
    bR = read(fd, buffer, BUFFER_FILEPOSIZIONI);
    if (bR == -1)
      errExit("<device> read da file_posizioni.txt fallita");
    if (bR != 0) {
      int i = 0;
      for (int col = 0; col < (BUFFER_FILEPOSIZIONI); col += 2) {
        printf("%3d", buffer[col] - 48);
        positionMatrix[row][i] = buffer[col] - 48;
        i++;
      }
      printf("\n");
    }

    row++;
  } while (bR != 0 && (row < LIMITE_POSIZIONI)); // iterazione fino a lettura di un valore dal file e row < limite 

  // Valore sentinella (usato in caso in cui non si raggiunga il limite massimo di righe [LIMITE_POSIZIONI])
  if (row < LIMITE_POSIZIONI - 1) {
    positionMatrix[row - 1][0] = 999;
  }

  /* Struttura di stampa matrice posizioni/
  for(int i = 0; i < LIMITE_POSIZIONI; i++){
  	for(int j = 0; j < 10; j++){
  		printf("%d \t", positionMatrix[i][j]);
  	}
  printf("\n");
  }
  */

  // chiusura del file descriptor
  close(fd);
}

