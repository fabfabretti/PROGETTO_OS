/// @file semaphore.h
/// @brief Contiene la definizioni di variabili e funzioni
///         specifiche per la gestione dei SEMAFORI.

#pragma once

/* errsemOpExit is a support function to manipulate a semaphore's value
 * of a semaphore set. semid is a semaphore set identifier, sem_num is the
 * index of a semaphore in the set, sem_op is the operation performed on sem_num
 */

union semun {
  int val;
  struct semid_ds * buf;
  unsigned short * array;
};

// Sostituisce la system call semop rendendola più agibile eliminando la necessità dei flag
void semOp(int semid, unsigned short sem_num, short sem_op);