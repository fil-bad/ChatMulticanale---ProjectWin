#pragma once
//
// Created by filippo on 25/09/18.
//

#ifndef CLIENT_CLIENT_H
#define CLIENT_CLIENT_H


#include <sys/types.h>
//#include <sys/socket.h>
//#include <netinet/in.h>
//#include <netinet/tcp.h>
//#include <arpa/inet.h>
//#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
//#include <strings.h>
//#include <unistd.h>
#include <errno.h>
//#include <pthread.h>
//#include <poll.h>
#include <signal.h>

#include <basetsd.h>

//#define WIN32_LEAN_AND_MEAN
//#include <Windows.h>
//#include <WinSock2.h>

//#include <Winerror.h>
//#include <WinSock2.h>	// NECESSARIO MANTENERE QUESTO ORDINE!
//#include <winsock.h>	// |
#include <windows.h>	// ^

#include <Mstcpip.h>

#include "tableFile.h"  // I locali vanno messi sotto, visto che includono a loro volta Windows.h
#include "mexData.h"


/* TIPI DI PACCHETTO*/

enum typePack {
	/** SYSTEM **/ success_p = 0,
	failed_p,
	mess_p,
	messSuccess_p,
	test_p,
	/** USER**/    login_p,
	logout_p,
	delUs_p,
	mkUser_p,
	dataUs_p,
	kConv_p,
	/** ROOM**/    mkRoom_p,
	joinRm_p,
	openRm_p,
	dataRm_p,
	leaveRm_p,
	delRm_p,
	exitRm_p
};

#define sendDim 40
#define wowDim 44

typedef struct metadata_ { //dim metadata = 96 byte
	size_t dim;
	int type; // dobbiamo definire dei tipi di comandi: es. 0 per il login => password in campo mex, ...
	char sender[sendDim];
	char whoOrWhy[wowDim];
} metadata;

typedef struct mail_ {
	metadata md;
	char *mex;
} mail;

#define dimDim 8
#define typeDim 8

typedef struct metadataChar_ { //dim metadataChar = 100 byte
	char dim[dimDim];
	char type[typeDim]; // dobbiamo definire dei tipi di comandi: es. 0 per il login => password in campo mex, ...
	char sender[sendDim];
	char whoOrWhy[wowDim];
} metadataChar;

typedef struct mailChar_ {
	metadataChar md;
	char *mex;
} mailChar;

typedef struct connection_ {
	SOCKET ds_sock;
	struct sockaddr_in sock;
} connection;

// PROTOTIPI DI FUNZIONE:

// GLOBALI

char *typeToText(int type);

connection *initSocket(UINT16 port, char *IP);

int keepAlive(SOCKET *ds_sock);

int readPack(SOCKET ds_sock, mail *pack);  // queste due funzioni prendono il pacchetto thread-specifico
int writePack(SOCKET ds_sock, mail pack); // ma all'interno contengono la struct mail con i dati
int testConnection(SOCKET ds_sock);

int fillPack(mail *pack, int type, int dim, void *mex, char *sender, char *whoOrWhy);

void printPack(mail *pack);

void printTextPack(mail *pack); //versione con solo testo;

///Client FUNCTION

int initClient(connection *c);

char *obtainStr(char *bufOut, int len);

int loginUser(SOCKET ds_sock, mail *pack);

int registerUser(SOCKET ds_sock, mail *pack);

table *initClientTable(table *tabChats, mail *pack);

void printChats(table *tabChats);

conversation *startConv(mail *pack, conversation *conv);

int chooseAction(char *command, connection *con, mail *pack, table *tabChats);

int createChat(SOCKET ds_sock, mail *pack, table *tabChats); // ## DONE
int deleteChat(SOCKET ds_sock, mail *pack, table *tabChats); // ## DONE
int leaveChat(SOCKET ds_sock, mail *pack, table *tabChats);  // ## DONE
int joinChat(SOCKET ds_sock, mail *pack, table *tabChats);   // ## DONE
int openChat(SOCKET ds_sock, mail *pack, table *tabChats);   // ## DONE

void helpChat(void);
void freeMexPack(mail *p);

void *thUserRX(connection *con);

void *thUserTX(connection *con);


#endif //CLIENT_CLIENT_H

/// ### DOCUMENTAZIONE ### ///

/* SO_KEEPALIVE
 *
 * http://tldp.org/HOWTO/TCP-Keepalive-HOWTO/usingkeepalive.html
 * Al paragrafo 3.1.1, e' mostrato come modificare il parametro temporale di sistema tramite CAT.
 * Sara' necessario ridurre i tempi per rendersene conto;
 * I cambiamenti dovranno essere impostati ad ogni avvio;
 *
 * E' POSSIBILE FARLO CON DEI VALORI DA MODIFICARE!!! VEDERE PARAGRAFO 4.2
 *
 */




