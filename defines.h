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
#include <signal.h>

/*
#include "err_exit.h"
#include "fifo.h"
#include "semaphore.h"
#include "shared_memory.h"

#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/sem.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>
include <signal.h>

#define ACK_LIST_SIZE 100
*/


#define BUFFER_FILEPOSIZIONI 20

void open_filePosition(char* path2file);

typedef struct {
  pid_t pid_sender;
  pid_t pid_receiver;
	int message_id;
	double max_distance;
	char message[256];
} message;