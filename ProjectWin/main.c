#include <stdio.h>
#include <string.h>
//#include <pthread.h>
//#include <semaphore.h>

#include <Windows.h>
#include <signal.h> // mantenere compliance con i segnali 

#include "client.h"
#include "fileSystemUtylity.h"
#include "avl.h"


//pthread_t tidRX, tidTX;
DWORD tidRX, tidTX;
HANDLE hanTX, hanRX;

connection *con;
mail packRX, packTX;

table *tabChats; //tabella locale delle chat
int chatEntry; // Index della tabella dove si trova la chat nella quale scriveremo quando saremo nella fase di messaggistica
conversation *conv;
//sem_t semConv;

HANDLE semConv;

avl_pp avlACK; // verranno messi i vessaggi in attesa di una risposta;
//sem_t semAVL;

HANDLE semAVL;

mex *messageTX, *messageRX;

int TypeMex = mess_p; //e' il tipo del messaggio, che sara' modificato dall'handler con exitRM

void closeHandler(int sig) {
	closesocket(con->ds_sock);
	WSACleanup();
	exit(-1);
}

void changerType(int sig) {
	TypeMex = exitRm_p;
	printf("To Exit from Room now click 'Any Button' and ENTER\n");

}

int clientDemo(int argc, char *argv[]) {
	char *storage = argv[1];
	printf("[1]---> Fase 1, aprire lo storage\n");
	int errorRet;
	errorRet = _chdir(storage);                        //modifico l'attuale directory di lavoro del processo
	if (errorRet != 0)    //un qualche errore nel ragiungimento della cartella
	{
		switch (errno) {
		case 2: //No such file or directory
			printf("directory non esistente, procedo alla creazione\n");
			errorRet = _mkdir(storage, 0777);
			if (errorRet == -1) {
				perror("mkdir fails");
				return -1;
			}
			else {
				printf("New directory create\n");
				errorRet = _chdir(storage);
				if (errorRet == -1) {
					perror("nonostante la creazione chdir()");
					return -1;
				}
			}
			break;
		default:
			perror("chdir");
			return -1;
		}
	}
	char curDirPath[100];
	_getcwd(curDirPath, 100);

	printf("Current Directory set:\n-->\tgetcwd()=%s\n\n", curDirPath);
	printf("[1]---> success\n\n");

	char buff[1024];
	con = initSocket((UINT16)strtol(argv[3], NULL, 10), argv[2]);

	if (initClient(con) == -1) {
		exit(-1);
	}
	//signal(SIGINT, closeHandler); // PER AIUTARE A GESTIRE LATO SERVER LA CHIUSURA INCONTROLLATA
	SetConsoleCtrlHandler((PHANDLER_ROUTINE)closeHandler, TRUE);


	printf("Connection with server done. ");
	mail *pack = malloc(sizeof(mail));

	/** PARTE LOGIN O CREATE **/
retry:
	printf("Please choose 'login'/'1' or 'register'/'2'\n>>> ");

	obtainStr(buff, 1024);

	if (strcmp(buff, "login") == 0 || strtol(buff, NULL, 10) == 1) {
		if (loginUser(con->ds_sock, pack) == -1) {
			perror("Login failed; cause:");
			return -1;
		}
	}
	else if (strcmp(buff, "register") == 0 || strtol(buff, NULL, 10) == 2) {
		int usid = registerUser(con->ds_sock, pack);
		if (usid == -1) {
			return -1;
		}
	}
	else {
		printf("Caso non supportato; riprovare\n");
		goto retry;
	}
	printf("<UserID>:<USER> = %s:%s\n", UserID, UserName);

	tabChats = initClientTable(tabChats, pack);
	if (tabChats == NULL) {
		printf("Errore apertura Tabella Chat.\n");
		return -1;
	}

	printf("\nWelcome. ");

	SetConsoleCtrlHandler((PHANDLER_ROUTINE)closeHandler, FALSE);

showChat: // label che permette di re-listare tutte le chat
	//signal(SIGINT, closeHandler); // PER AIUTARE A GESTIRE LATO SERVER LA CHIUSURA INCONTROLLATA
	SetConsoleCtrlHandler((PHANDLER_ROUTINE)closeHandler, TRUE);

	printf("You can talk over following chat:\n");

	printChats(tabChats);

	printf("\nPlease choose one command or the relative number: (otherwise help() will be shown)\n\n");
	printf("\t'1'/'createChat'\t'4'/'joinChat'\t\t'$p'/'printTab'\n"
		"\t'2'/'deleteChat'\t'5'/'openChat'\t\t'$e'/'exitProgram'\n"
		"\t'3'/'leaveChat'\t\t\n\n>>> ");

	obtainStr(buff, 1024);

	if ((strcmp(buff, "exitProgram") == 0 || (strcmp(buff, "$e") == 0))) {
		closesocket(con->ds_sock);
		WSACleanup();
		return(0);
	}

	chatEntry = chooseAction(buff, con, pack, tabChats);
	if (chatEntry == -1) goto showChat;

	if (!(strcmp(buff, "openChat") == 0 || strtol(buff, NULL, 10) == 5)) {
		goto showChat;
	}

	printf("Benvenuto nella chat %s.\n", tabChats->data[chatEntry].name);

	//signal(SIGINT, changerType); //inizio a gestire i l'handler per l'uscita di messaggio
	SetConsoleCtrlHandler((PHANDLER_ROUTINE)closeHandler, FALSE);
	SetConsoleCtrlHandler((PHANDLER_ROUTINE)changerType, TRUE);

	conv = startConv(pack, conv); //scarichiamo tutta la conversazione in locale
	if (conv == NULL) {
		printf("Conv not initialized.\n");
		return -1;
	}
	//* INIZIALIZZO OGNI VOLTA L'AVL SE NON ERA STATO CREATO*//

	avlACK = init_avl();
	printf("Avl initialized.\n");

	printf("Entro nella room...\n");
	
	semConv = CreateSemaphoreA(NULL, 0, 1, NULL);
	if (!semConv) {
		printf("failed to create semConv");
		return -1;
	}

	semAVL = CreateSemaphoreA(NULL, 1, 1, NULL);
	if (!semAVL) {
		printf("failed to create semAVL");
		return -1;
	}

	printf("Semaphore passed...\n");

	hanTX = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)thUserTX, con, NORMAL_PRIORITY_CLASS, &tidTX);
	if (hanTX == NULL) ExitProcess(-1);
	hanRX = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)thUserRX, con, NORMAL_PRIORITY_CLASS, &tidRX);
	if (hanRX == NULL) ExitProcess(-1);
	
	printf("Going to wait...\n");

	WaitForSingleObject(semConv, INFINITE);
	
	printf("Started Again...\n");

	TerminateThread(hanTX, NULL);
	CloseHandle(hanTX);

	TerminateThread(hanRX, NULL);
	CloseHandle(hanRX);

	printf("Threads deleted...\n");

	// Elimino l'avl della conversazione, non piu' necessario
	destroy_avl(avlACK);

	//signal(SIGINT, SIG_DFL);
	SetConsoleCtrlHandler((PHANDLER_ROUTINE)changerType, FALSE);


	printf("Signal restored...\n");

	goto showChat;
}

void *thUserRX(connection *con) {
	do {
		if (readPack(con->ds_sock, &packRX) == -1) {
			exit(-1);
			//break;
		}
		if (packRX.md.type == delRm_p) { //potrei riceverlo di un'altra chat
			int entCH = (int)strtol(packRX.md.whoOrWhy, NULL, 10);
			delEntry(tabChats, entCH);
			if (entCH == chatEntry) break; // se e' della chat esco dalla RX
			else continue;             // se e' di un'altra, continuo
		}
		if (packRX.md.type == messSuccess_p) {
			//sem_wait(&semAVL);
			WaitForSingleObject(semAVL, INFINITE);

			delete_avl_node(avlACK, (int)strtol(packRX.md.whoOrWhy, NULL, 10));
			//sem_post(&semAVL);
			ReleaseSemaphore(semAVL, 1, NULL);

			printf("\t\t\t>>Message %s server Confirmed<<\n", packRX.md.whoOrWhy);
			continue;
		}
		if (packRX.md.type == failed_p) {
			printf("Failed received, cause: %s\n", packRX.md.whoOrWhy);
			continue;
		} //ignoro questo stato
		if (packRX.md.type != mess_p) {
			printf("Unexpected pack; going to main menu...\n");
			printPack(&packRX);
			break;
		}

		//printPack (&packRX);
		printMexBuf(packRX.mex, STDOUT_FILENO);
		//printTextPack(&packRX);

		/* PARTE INSERIMENTO IN CONV DEI MESSAGGI*/
		messageRX = makeMex(packRX.mex, (int)strtol(UserID, NULL, 10));
		if (addMex(conv, messageRX) == -1) {
			printf("Error writing mex on conv in RX.\n");
			break;
		}
	} while (packRX.md.type != delRm_p);

	if (packRX.md.type == delRm_p) delEntry(tabChats, chatEntry);

	freeMexPack(&packTX);
	freeMexPack(&packRX);

	//sem_post(&semConv);
	ReleaseSemaphore(semConv, 1, NULL);
	return NULL;
}

void *thUserTX(connection *con) {

	char buff[1024];
	int codMex = 0;
	char WorW[wowDim];

	char userBuff[sendDim];
	sprintf(userBuff, "%s:%s", UserID, UserName); // UserID:UserName
	userBuff[sendDim - 1] = '\0';
	TypeMex = mess_p;

	printf("Inserire un messaggio ('-q' per terminare):");

	do {
		printf("\n>>> ");
		obtainStr(buff, 1024); //messaggio da mandare

		sprintf(WorW, "%d", codMex); //codice "univoco" per il messaggio
		WorW[wowDim - 1] = '\0';
		codMex++;

		if (strcmp(buff, "-q") == 0) TypeMex = exitRm_p;

		if (TypeMex == exitRm_p) { // nel caso volessimo uscire NON mandiamo il messaggio attualmente in scrittura
			fillPack(&packTX, exitRm_p, 0, NULL, userBuff, tabChats->data[chatEntry].name);
			if (writePack(con->ds_sock, packTX) == -1) {
				if (errno == EPIPE) exit(-1);
			}
			printf("Exit pack inviato\n");
			break;
		}
		else { // altrimenti mandiamo come tipo mess_p e il messaggio scritto in precedenza
			fillPack(&packTX, mess_p, (int)strlen(buff) + 1, buff, userBuff, WorW);
			//sem_wait(&semAVL);
			WaitForSingleObject(semAVL, INFINITE);

			insert_avl_node(avlACK, (int)strtol(packTX.md.whoOrWhy, NULL, 10),
				(int)strtol(packTX.md.whoOrWhy, NULL, 10));
			
			//sem_post(&semAVL);
			ReleaseSemaphore(semAVL, 1, NULL);

			if (writePack(con->ds_sock, packTX) == -1) {
				if (errno == EPIPE) exit(-1);
				//sem_wait(&semAVL);
				WaitForSingleObject(semAVL, INFINITE);

				delete_avl_node(avlACK, (int)strtol(packTX.md.whoOrWhy, NULL, 10));
				
				//sem_post(&semAVL);
				ReleaseSemaphore(semAVL, 1, NULL);

				break;
			}
		}

		//sem_wait(&semAVL);
		WaitForSingleObject(semAVL, INFINITE);
		
		if ((**(avlACK)).height > 4) { // quindi almeno 2^5 = 32 success pendenti
			printf("Attention, AVL height is %d, there could be some problems on the server.\n",
				(**(avlACK)).height);
		}
		//sem_post(&semAVL);
		ReleaseSemaphore(semAVL, 1, NULL);
		
		printPack(&packTX);
		printTextPack(&packTX);

		/* PARTE INSERIMENTO IN CONV DEI MESSAGGI*/
		messageTX = makeMex(packTX.mex, (int)strtol(UserID, NULL, 10));
		if (addMex(conv, messageTX) == -1) {
			printf("Error writing mex on conv in TX.\n");
			break;
		}
	} while (packTX.md.type != exitRm_p);
	freeMexPack(&packTX);
	freeMexPack(&packRX);
	
	//sem_post(&semConv);
	ReleaseSemaphore(semConv, 1, NULL);
	return NULL;
}

void helpProject() {
	printf("I parametri Client sono:\n");
	printf("[PATH_save] [IP] [port]\tMi collego al server a IP e porta specificati (IP = 127.0.0.1 in locale)\n");
}

int main(int argc, char *argv[]) {
	fdDebug = 1;
	if (argc == 4) {
		if (clientDemo(argc, argv) == -1) {
			return -1;
		}
	}
	else helpProject();
	return 0;
}