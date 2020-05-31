/// @file defines.h
/// @brief Contiene la definizioni di variabili
///         e funzioni specifiche del progetto.

#pragma once

#include "err_exit.h"
#include "fifo.h"
#include "semaphore.h"
#include "shared_memory.h"


#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <math.h>
#include <signal.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/sem.h>
#include <sys/shm.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <time.h>
#include <sys/types.h>
#include <unistd.h>

// Define 

// Buffer utile alla read del file posizioni
#define BUFFER_FILEPOSIZIONI 20

// Dimensione della ACK LIST
#define ACK_LIST_SIZE 100

// Limite righe lette nel file posizioni
#define LIMITE_POSIZIONI 100

// Matrice delle coordinate dei devices nel tempo
int positionMatrix[LIMITE_POSIZIONI][10];



// Strutture dati

//Struttura dati dei messaggi
typedef struct {
  pid_t pid_sender;
  pid_t pid_receiver;
  int message_id;
  double max_distance;
  char message[256];
} message;


// struttura contenente matrice (-> board)
typedef struct {
  pid_t board[10][10];
} board_t;


// struttura di un singolo messaggio acknowledge
typedef struct {
  pid_t pid_sender;
  pid_t pid_receiver;
  int message_id;
  time_t timestamp;
} Acknowledgment;

/* 
Tale struttura serve per mantenere in memoria i messaggi inseriti nella fifo.
Se non venisse usata, il messaggio, una volta letto nell'altra estremità, sarebbe perso.
*/
typedef struct {
 	message msg;

	// Flag di controllo (se 0 non ancora inviato la prima volta)
	int firstSent;
} inbox_t;

// Funzioni
double distanceCalculator(int x1, int y1, int x2, int y2);
void open_filePosition(char * path2file);


