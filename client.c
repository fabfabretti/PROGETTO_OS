/// @file client.c
/// @brief Contiene l'implementazione del client.

#include "defines.h"
#include "err_exit.c"


#define TICK "[\033[1;32m✓\033[0m]"

////////////////////////////////////////////////
//       			VARIABILI GLOBALI	      			  //
////////////////////////////////////////////////

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
		errExit("Client can't open the fifo");

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

	//DEBUG: stampa path creato x file
	printf("\nPath del file: %s", path2file);

	int fd_out_file = open(path2file, O_CREAT | O_EXCL | O_TRUNC | O_WRONLY , S_IRWXU |  S_IRWXG |   S_IRWXO );
	if(fd_out_file == -1)
		errExit("Couldn't create output file");

	// Buffer contente il messaggio che verrà scritto nel file di out
	char bufferFileTxt[100000];

	// Dopo %s non serve \n perchè è contenuto nel messaggio
	sprintf(bufferFileTxt, "Messaggio %d: %sLista acknowledgment:\n", msg.message_id, msg.message);


	for(int j = 0; j < 5; j++){

		struct tm *time = localtime(&buffer.ack_msgq_array[j].timestamp);


		char tmp[500];

		char tmpTime[500];
		strftime(tmpTime, 100, "%Y-%m-%d %H:%M:%S", time);
		
		sprintf(tmp, "%d, %d, %s\n", buffer.ack_msgq_array[j].pid_sender,buffer.ack_msgq_array[j].pid_receiver, tmpTime);
		
		sprintf(bufferFileTxt, "%s%s", bufferFileTxt, tmp);
	}

	if(write(fd_out_file, bufferFileTxt, sizeof(char) * strlen(bufferFileTxt))==-1)
		errExit("Client couldn't write on output file.");
		


	//DEBUG: stampa del buffer
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

	//controllo semplice: > 0
	if(message_id<=0){
		printf("[\033[1;31mx\033[0m] Message ID must be greater than 0!\nPlease insert a valid message id: ");
		return 1;
		}

	// Generazione stringa del file di controllo
	char pathToFifo[20];
	sprintf(pathToFifo,"./fifo/%d",message_id);

	int fd_message_checker = open(pathToFifo, O_RDONLY | O_CREAT | O_EXCL,  S_IRWXU );
	if(fd_message_checker==-1){
		printf("[\033[1;31mx\033[0m] Message ID is already in use!\nPlease insert a valid message id: ");
		return 1 ;
		}

	return 0;
/*
	int fd_message_checker = 0, message_id_file;

	char buffer_id[1];
	char tmp[10];

	fd_message_checker = open("./fifo/message_checker.txt", O_RDWR | O_CREAT ,  S_IRWXU |  S_IRWXG |  S_IRWXO );

	if(fd_message_checker == -1)
			errExit("Message ID checker couldn't open file");
			
	ssize_t bR = 0;

	int primogiro = 1;

	do{
		printf("### %d\n\n", fd_message_checker);
		bR = read(fd_message_checker, &buffer_id, sizeof(buffer_id));

		// Primo giro, esce subito
		if(bR == 0 && primogiro == 1){

			sprintf(tmp,"%d$", message_id);

			if(write(fd_message_checker, &tmp, sizeof(char)*strlen(tmp)) == -1)
				errExit(("Write in message_checker.txt failed."));
			//scrivi il nuovo id
			return 0;	
			}

		primogiro = 0;


		// Check errore
		if(bR == -1)
			errExit("Read from file message_checker.txt failed");
		
		// Composizione del pid
		else if (bR != 0 && buffer_id[0] != '$'){
			sprintf(tmp, "%s%c", tmp, *buffer_id);
		}
		// Trasformazione in intero
		else if(bR != 0 && buffer_id[0] == '$'){
			message_id_file = atoi(tmp);

			// Check unicità 
			if(message_id == message_id_file)
				return 1;


		sprintf(tmp, "%s","");				
		}
		
	}while(bR != 0);


	sprintf(tmp,"%d$", message_id);

	if(write(fd_message_checker, tmp, sizeof(char) * strlen(tmp)) == -1)
		errExit(("Write in message_checker.txt failed"));

	return 0;
	*/
	
	
}
