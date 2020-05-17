/// @file semaphore.c
/// @brief Contiene l'implementazione delle funzioni
///         specifiche per la gestione dei SEMAFORI.

#include "err_exit.h"
#include "semaphore.h"
#include <sys/sem.h>

#include "semaphore.h"
/*
void semOp (int semid, unsigned short sem_num, short sem_op) {
    struct sembuf sop = {.sem_num = sem_num, .sem_op = sem_op, .sem_flg = 0};

    if (semop(semid, &sop, 1) == -1)
        errExit("semop failed");
}

*/


void semOp (int semid, unsigned short sem_num, short sem_op) {
    struct sembuf sop[1];
    sop[0].sem_num=sem_num;
    sop[0].sem_op=sem_op;
    sop[0].sem_flg=0;

    if (semop(semid,sop,1) == -1)
		errExit("<semaphore> semop failed");
}