/// @file client.c
/// @brief Contiene l'implementazione del client.

#include "defines.h"
#include <stdio.h>
#include <stdlib.h>
#include "err_exit.c"
#include <string.h>

/*
i client
• Il client deve:

- chedere all’utente le informazioni necessarie per inviare un messaggio ad un device:
		• Inserire pid device a cui inviare il messaggio (pid_t pid_device)
		• Inserire id messaggio (int message_id)
		• Inserire messaggio (char* message)
		• Inserire massima distanza comunicazione per il messaggio (double max_distance)
poi prepara il messaggio (inserendo anche il proprio pid) e lo invia alla fifo del relativo device.)

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

	//controllo utilizzo!
	if(argc != 2){
		printf("[x] Usage : %s msg_queue_id\n",argv[0]);
		exit(1);
	}



  ////////////////////////////////////////////////
  //       			VARIABILI					      			  //
  ////////////////////////////////////////////////



	message msg;



  ////////////////////////////////////////////////
  //   				CODICE PROGRAMMA								  //
  ////////////////////////////////////////////////



	// Assegnazione della chiave (linea di comando)
	msg.message_id = atoi(argv[1]);

	// Assegnazione pid del sender
	msg.pid_sender = getpid();


	// Assegnazione pid receiver
	printf("[+] Insert the pid of the device which will receive the message: ");
	scanf("%d",&msg.pid_receiver);
	
	// Assegnazione del messaggio	
	printf("[+] Insert the message to send: ");
	// il while serve per non considerare lo '\n' contenuto nel buffer di input (causa scanf precedente)
	while(getchar() != '\n');
	fgets(msg.message, 256, stdin);


	// Assegnazione massima distanza percorribile dal messaggio
	printf("[+] Insert the maximum distance the message will reach: ");
	scanf("%lf",&msg.max_distance);

	//debug
	//printf("[?] Mando messaggio [%s] a device %d",msg.message,msg.pid_receiver);

	char path2fifo[100];
	
	char * fifopathbase = "./fifo/dev_fifo.";

	sprintf(path2fifo, "./fifo/dev_fifo.%d\n", msg.pid_receiver);

	// Creazione fifo (col nome appropriato appena trovato.)
	int fd = open(path2fifo, O_WRONLY);
	//se ci sono errori

	write(fd, &msg, sizeof(msg));
  


	close(fd);
	return 0;
}