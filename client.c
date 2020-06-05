/// @file client.c
/// @brief Contiene l'implementazione del client.

#include "defines.h"
#include "err_exit.c"

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
  printf("[+] Insert the ID of the messasge (must be > 0): ");
  scanf("%d", &msg.message_id);
	while(msg.max_distance < 0){
		printf("\nPleas, insert a value higher than 0.");
  	scanf("%d", &msg.message_id);
	}

  // Assegnazione MESSAGGIO
  printf("[+] Insert the message to send: ");

  // il while serve per non considerare lo '\n' contenuto nel buffer di input (causa scanf precedente)
  while (getchar() != '\n');
  fgets(msg.message, 256, stdin);

  // Assegnazione MAX DISTANCE
  printf("[+] Insert the maximum distance the message will reach: ");
  scanf("%lf", &msg.max_distance);
	
	while(msg.max_distance < 0){
		printf("\nPleas, insert a value higher than 0.");
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
	if(fd == -1)
		errExit("Client can't open the fifo");

  // Scrittura del messaggio nella fifo
  write(fd, &msg, sizeof(msg));

  printf("[✓] Message sent to device <%d>\n", msg.pid_receiver);

  // Chiusura fifo
  close(fd);

  return 0;
}