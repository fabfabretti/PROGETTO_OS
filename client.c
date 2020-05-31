/// @file client.c
/// @brief Contiene l'implementazione del client.

#include "defines.h"


/*

 Il client poi resta in attesa del messaggio contenente gli acknowledgment dalla
coda di messaggi. 

Ricevuto il messaggio il client salva su file la lista di acknowledgment e termina.


Infine si termina il server, ack_manager e devices inviando un segnale SIGTERM al processo server
il quale gestisce la chiusura dei processi figlio e la rimozione/chiusura dei meccanismi di
comunicazione tra processi (i.e., fifo, memoria condivisa, code messaggi, semafori, etc).

- aprire la relativa fifo in scrittura

- inviare il messaggio e poi attendere dalla coda di messaggi la relativa lista di acknowledgment. 

- Una volta ricevuta, deve stamparla su file e terminare.

• Moltepici client possono essere eseguiti in modo concorrente per inviare più messaggi (con
diverso id) a più device.
• Il message_id passato dall’utente al client deve essere univoco
*/

////////////////////////////////////////////////
//       			VARIABILI GLOBALI	      			  //
////////////////////////////////////////////////

////////////////////////////////////////////////
//       	 PROTOTIPI FUNZIONI	         			  //
////////////////////////////////////////////////

////////////////////////////////////////////////
//     				  	 MAIN				         			  //
////////////////////////////////////////////////

int main(int argc, char * argv[]) {

  //controllo del numero di argomenti dal terminale
  if (argc != 2) {
    printf("[x] Usage : %s msg_queue_id\n", argv[0]);
    exit(1);
  }

  ////////////////////////////////////////////////
  //       			VARIABILI					      			  //
  ////////////////////////////////////////////////

  // Struttura dei messaggi (vedi defines.h -> message)
  message msg;

  ////////////////////////////////////////////////
  //   				CODICE PROGRAMMA								  //
  ////////////////////////////////////////////////

  // Assegnazione KEY (ottenuta dal terminale)
  msg.message_id = atoi(argv[1]);

  // Assegnazione PID SENDER
  msg.pid_sender = getpid();

  // Assegnazione PID RECEIVER
  printf("[+] Insert the pid of the device which will receive the message: ");
  scanf("%d", &msg.pid_receiver);

  // Assegnazione ID MESSAGGIO
  printf("[+] Inserire id messaggio:");
  scanf("%d", &msg.message_id);

  // Assegnazione MESSAGGIO
  printf("[+] Insert the message to send: ");

  // il while serve per non considerare lo '\n' contenuto nel buffer di input (causa scanf precedente)
  while (getchar() != '\n');
  fgets(msg.message, 256, stdin);

  // Assegnazione MAX DISTANCE
  printf("[+] Insert the maximum distance the message will reach: ");
  scanf("%lf", &msg.max_distance);
	while(msg.max_distance < 0){
		printf("\nGentilmente inserisci una distanza maggiore di 0!");
  	scanf("%lf", &msg.max_distance);
	}

  // DEBUG:
  //printf("[?] Mando messaggio [%s] a device %d",msg.message,msg.pid_receiver);

  // Ottenimento del path della fifo desiderata
  char path2fifo[100];

  char * fifopathbase = "./fifo/dev_fifo.";

  sprintf(path2fifo, "./fifo/dev_fifo.%d\n", msg.pid_receiver);

  // Appertura della fifo
  int fd = open(path2fifo, O_WRONLY);

  // Scrittura del messaggio nella fifo
  write(fd, &msg, sizeof(msg));

  printf("[✓] Message [hopefully] sent to device %d\n", msg.pid_receiver);

  // Chiusura fifo
  close(fd);

  return 0;
}