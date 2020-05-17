/// @file defines.h
/// @brief Contiene la definizioni di variabili
///         e funzioni specifiche del progetto.

#pragma once


#include <stdlib.h>
#include <stdio.h>
#include "err_exit.h"
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#define BUFFER_FILEPOSIZIONI 20

void open_filePosition(char* path2file);