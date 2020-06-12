/// @file err_exit.c
/// @brief Contiene l'implementazione della funzione di stampa degli errori.

#include "defines.h"



void errExit(const char * string) {
	char msg[100];

	sprintf(msg,"[\033[1;31mx\033[0m] %s",string);

  perror(msg);
  exit(1);
}