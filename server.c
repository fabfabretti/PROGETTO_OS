/// @file server.c
/// @brief Contiene l'implementazione del SERVER.

#include "err_exit.h"
#include "defines.h"
#include "shared_memory.h"
#include "semaphore.h"
#include "fifo.h"
#include <unistd.h>

#include <stdio.h>

#include <sys/shm.h>
#include <sys/types.h>

// Lezione fraccaroli 2:06:30
// allocazione shared_memory (board e shared list)
// attach  shared_memory create
// il puntatore alla scacchiera lo da figli. (codice identico a es. share memory/semfory/ecc.)
// prima fork ack_manager poi for loop e fork device

int main(int argc, char *argv[])
{
	// VARIABLES

	// board
	int shm_boardId = 0; // Chiave di accesso
	pid_t *boardB;       // Puntatore a prima cella array


	// CODE 
	//TODO: ack_manager

	//TODO: creazione, inizializzazione semafori

	// Allocazione shared memory per la board
	printf("\n<Server> Allocation shared memory board [...]");
	shm_boardId = alloc_shared_memory(IPC_PRIVATE, 100 * sizeof(pid_t));
	//crea il segmento di memoria condivisa da qualche parte nella memoria.

	// Attachnment segmento shared memory della board
	printf("\n<Server> Attachement shared memory board [...]");
	boardB = (pid_t *)get_shared_memory(shm_boardId, 0);

	//TODO: Fork per i 5 figli
	printf("\n<Server> Creazione 5 device (fork). [...] \n");

	for (int child = 0; child < 5; child++)
	{

			pid_t pid = fork();

			if (pid == -1)
					errExit("<Server> errore creazione device");
			
			else
			{
					if (pid == 0)
					{
							// Codice eseeguito dal figlio
					}
			}

			exit(0); //??
	}

	//TODO: cancellazione semafori

	// Detach shared memory board
	printf("\n<Server> Detaching shared memory board [...]");
	free_shared_memory(boardB);

	// Deleting shared memory board
	printf("\n<Server> Remoiving shared memory board [...]\n");
	remove_shared_memory(shm_boardId);
}