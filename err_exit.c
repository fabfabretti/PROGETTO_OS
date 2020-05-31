/// @file err_exit.c
/// @brief Contiene l'implementazione della funzione di stampa degli errori.

#include "defines.h"



void errExit(const char * msg) {
  perror(msg);
  exit(1);
}