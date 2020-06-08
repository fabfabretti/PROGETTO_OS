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


// Variabili
// - - - - - - - - - - - - - - - 
  // Struttura dei messaggi (vedi defines.h -> message)
  message msg;


// INPUT INFORMAZIONI
// - - - - - - - - - - - - - - - 

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
		printf("\nPlease, insert a value higher than 0.");
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
		printf("\nPlease, insert a value higher than 0.");
  	scanf("%lf", &msg.max_distance);
	}


// SCRITTURA SU FIFO
// - - - - - - - - - - - - - - - 
  char path2fifo[100];

  char * fifopathbase = "./fifo/dev_fifo.";

  sprintf(path2fifo, "./fifo/dev_fifo.%d\n", msg.pid_receiver);

  // Appertura della fifo
  int fd = open(path2fifo, O_WRONLY);
	if(fd == -1)
		errExit("Client can't open the fifo");

  // Scrittura del messaggio nella fifo
  write(fd, &msg, sizeof(msg));

  printf("[\033[1;32m✓\033[0m] Message sent to device <%d>\n\n", msg.pid_receiver);

  // Chiusura fifo
  close(fd);


// LETTURA SU MSGQUEUE
// - - - - - - - - - - - - - - - 


int msg_id=msgget(atoi(argv[1]),S_IRUSR);
if(msg_id==-1)
	errExit("Couldn't open message queue");

printf("[+] <Client> Waiting for response...");

ack_msgq buffer;
if(msgrcv(msg_id,&buffer,sizeof(ack_msgq),msg.message_id,0)==-1)
	errExit("Client couldn't read message queue");

printf("[\033[1;32m✓\033[0m] <Client> Message received correctly!\n");
  return 0;
}