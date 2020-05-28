/// @file defines.c
/// @brief Contiene l'implementazione delle funzioni
///         specifiche del progetto.

#include "defines.h"
#include <stdlib.h>
#include <stdio.h>
#include "err_exit.h"
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <math.h>

#define BUFFER_FILEPOSIZIONI 20

// La funzione calcola la distanza euclidea tra due device sulla board
double distanceCalculator(int x1, int y1, int x2, int y2){

	return sqrt((pow((x2 - x1), 2) + (pow((y2 - y1), 2))));
}


