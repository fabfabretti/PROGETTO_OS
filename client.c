/// @file client.c
/// @brief Contiene l'implementazione del client.

#include "defines.h"
#include "err_exit.c"


#define TICK "[\033[1;32m✓\033[0m]"

	////////////////////////////////////////////////
	//       	 PROTOTIPI FUNZIONI	         			  //
	////////////////////////////////////////////////

int messageIdChecker(int message_id);

	////////////////////////////////////////////////
	//     				  	 MAIN				         			  //
	////////////////////////////////////////////////

int main(int argc, char * argv[]) {

  //controllo del numero di argomenti dal terminale
  if (argc != 2) {
    printf("[x] Usage : %s msg_queue_id\n", argv[0]);
    exit(1);
  }


	// VARIABILI
	// - - - - - - - - - - - - - - - 
  // Struttura dei messaggi (vedi defines.h -> message)
  message msg;


	// INPUT INFORMAZIONI
	// - - - - - - - - - - - - - - - 

  // Assegnazione KEY (da terminale)
  msg.message_id = atoi(argv[1]);

  // Assegnazione PID SENDER
  msg.pid_sender = getpid();

  // Assegnazione PID RECEIVER
  printf("[+] Insert the pid of the device which will receive the message: ");
  scanf("%d", &msg.pid_receiver);

  // Assegnazione ID MESSAGGIO
  printf("[+] Insert the ID of the message (must be > 0): ");
	do{
  	scanf("%d", &msg.message_id);
	}	while(messageIdChecker(msg.message_id) == 1);

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
		errExit("Client can't open fifo");

  // Scrittura del messaggio nella fifo
  write(fd, &msg, sizeof(msg));

  printf("[\033[1;32m✓\033[0m] Message sent to device with pid %d\n\n", msg.pid_receiver);

  // Chiusura fifo
  close(fd);




	// LETTURA DA MSGQUEUE
	// - - - - - - - - - - - - - - - 


	int msg_id=msgget(atoi(argv[1]), S_IRUSR);
	if(msg_id == -1)
		errExit("Couldn't open message queue");

	printf("\033[0;93m<Client> Waiting for response...\033[0m");
	fflush(stdout);
	ack_msgq buffer;
	if(msgrcv(msg_id, &buffer, sizeof(ack_msgq), msg.message_id, 0) == -1)
		errExit("Client couldn't read message queue");

	printf("\033[0;93m\n<Client> I got a message! :O\033[0m\n");
	

	// CREAZIONE FILE
	// - - - - - - - - - - - - - - - 
	
	// Cartella che conterrà i file di output
	mkdir("./output", S_IRUSR | S_IWUSR | S_IXUSR);

	char path2file[100];

	sprintf(path2file, "./output/out_message_%d.txt", msg.message_id);


	int fd_out_file = open(path2file, O_CREAT | O_EXCL | O_TRUNC | O_WRONLY , S_IRWXU |  S_IRWXG |   S_IRWXO );
	if(fd_out_file == -1)
		errExit("Couldn't create output file");


	// Costruzione delle stringhe!!

	// Buffer che conterrà il messaggio finale
	char bufferFileTxt[1000];

	// Stringa iniziale col messaggio
	char tmpIntro[400];
	sprintf(tmpIntro, "Messaggio %d: %sLista acknowledgment:\n", msg.message_id, msg.message);

	//Stringa degli ack (PIDs + timestamps)
	//Uso 5 variabili separate perché usando un array di 5 stringhe me le sovrascrive :(
	char tmpAck0 [100];
	char tmpAck1 [100];
	char tmpAck2 [100];
	char tmpAck3 [100];
	char tmpAck4 [100];
	
	//Stringa di un timestamp
	char tmpTime[50];
	
	for(int j = 0; j < 5; j++){

		//Creazione stringa del timestamp
		struct tm *time = localtime(&buffer.ack_msgq_array[j].timestamp);
		strftime(tmpTime, 100, "%Y-%m-%d %H:%M:%S", time);
		
		//Creazione riga di un ack (dunque, PID sender/receiver + stringa timestamp)
		
		char *dest;
		
		switch(j){
		case 0: dest=tmpAck0;
						break;
		case 1: dest=tmpAck1;
						break;
		case 2: dest=tmpAck2;
						break;
		case 3: dest=tmpAck3;
						break;
		case 4: dest=tmpAck4;
						break;
		}
		
		
		sprintf(dest, "%d, %d, %s\n", buffer.ack_msgq_array[j].pid_sender,buffer.ack_msgq_array[j].pid_receiver, tmpTime);
		
		
	}
	
	

		sprintf(bufferFileTxt, "%s%s%s%s%s%s",tmpIntro,tmpAck0, tmpAck1, tmpAck2, tmpAck3, tmpAck4);
		printf("%s\n",bufferFileTxt);
		
	if(write(fd_out_file, bufferFileTxt, sizeof(char) * strlen(bufferFileTxt))==-1)
		errExit("Client couldn't write on output file.");
		

	printf("\033[0;93m<Client> Output generato:\033[0m\n%s\n", bufferFileTxt);
	printf("\n%s Client terminato correttamente.\n",TICK);
  return (0);
}

/*
La funzione controlla l'unicità di un message id passato dall'utente.
La funzione legge un carattere per volta dal file e usa il simbolo $ per dividere gli ID tra di loro.

I valori del risultato possono essere:
 - 0 -> l'id passato è accettabile;
 - 1 -> l'id passato è già in uso o è < 0
*/
int messageIdChecker(int message_id){

	//Primo controllo: ID > 0 ?
	// - - - - - - - - - - - - -
	if(message_id <= 0){
		printf("[\033[1;31mx\033[0m] Message ID must be greater than 0!\nPlease insert a valid message id: ");
		return 1;
		}


	//Secondo controllo: ID è univoco ?
	// - - - - - - - - - - - - -
	char pathToFifo[20];
	sprintf(pathToFifo,"./fifo/%d",message_id);
	errno=0;
	int fd_message_checker = open(pathToFifo, O_RDONLY | O_CREAT | O_EXCL,  S_IRWXU );
	if(fd_message_checker==-1 && errno==17){
		printf("[\033[1;31mx\033[0m] Message ID is already in use!\nPlease insert a valid message id: ");
		return 1 ;
		}
	if(fd_message_checker==-1 && errno!=17)
		errExit("Client coulndn't check ID correctly");

	return 0;	
}
