/// @file defines.c
/// @brief Contiene l'implementazione delle funzioni
///         specifiche del progetto.

#include "defines.h"

// La funzione calcola la distanza euclidea tra due device sulla board
double distanceCalculator(int x1, int y1, int x2, int y2) {

  return sqrt((pow((x2 - x1), 2) + (pow((y2 - y1), 2))));
}


/*
Apre il file delle posizioni.
*/
void open_filePosition(char * path2file) {

  //Apertura file di input.
  int fd = open(path2file, O_RDONLY);
  if (fd == -1)
    errExit("[x] <Device> open file_posizioni.txt failed");

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
      errExit("\n\n[x] <Device> Reading of file_posizioni.txt failed");

    if (bR != 0) {
      int i = 0;
      for (int col = 0; col < (BUFFER_FILEPOSIZIONI); col += 2) {
        positionMatrix[row][i] = buffer[col] - 48;
        i++;
      }

      // Check posizioni doppie per riga del file_posizioni.txt
      for (int j = 0; j < 10; j += 2) {
        for (int k = j; k < 10; k += 2) {
          if (positionMatrix[row][j] == positionMatrix[row][k] && j != k) {
            if (positionMatrix[row][j + 1] == positionMatrix[row][k + 1])
              errExit("[x] <Server> The are devices in the same position at the same time.");
          }
        }
      }

      //printf("\n[✓] file_posizioni.txt: the coordinates in the file are not illegal");
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