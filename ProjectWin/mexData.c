//
// Created by alfylinux on 27/09/18.
//

#include "mexData.h"

///Funzioni di interfaccia

FILE *openConfStream(char *path)
{
	FILE *f = fopen(path, "w+x"); // exclusive access on file, fail if it exists

	if (f == NULL)
	{
		f = fopen(path, "r+");

		printf("[fConvopen] r+ executed, %p\n", f);
		if (f == NULL) {
			printf("[FConvOPEN] failed r+ opening\n");
			return NULL;
		}
	}
	return f;
}

conversation *openConf(char *convPath) {
	FILE *f = openConfStream(convPath);
	if (f == NULL) {
		perror("mex open error:");
		return NULL;
	}
	return loadConvF(f);
}

int addMex(conversation *c, mex *m) {
	if (saveNewMexF(m, c->stream)) {
		perror("error during write :");
		return -1;
	}
	c->head.nMex++;
	c->mexList = reallocarray(c->mexList, c->head.nMex, sizeof(mex *));
	c->mexList[c->head.nMex - 1] = m;
	if (overrideHeadF(&c->head, c->stream)) {
		return -1;
	}
	return 0;
}

mex *makeMex(char *text, int usId) {
	/// text su un buf temporaneo
	mex *m = malloc(sizeof(mex));
	if (m == NULL) {
		return NULL;
	}
	m->info.usId = usId;
	m->info.timeM = currTimeSys();
	m->text = malloc(strlen(text) + 1);
	strcpy(m->text, text);
	return m;
}

mex *makeMexBuf(size_t len, char *bufMex) {
	bufMex[len - 1] = 0; //non dovrebbe essere necessario, per sicurezza
	/// text su un buf temporaneo
	mex *m = malloc(sizeof(mex));
	if (m == NULL) {
		return NULL;
	}
	m->text = malloc(len - sizeof(mexInfo));
	if (!m->text) {
		free(m);
		return NULL;
	}
	memcpy(&m->info, bufMex, sizeof(mexInfo));
	strcpy(m->text, bufMex + sizeof(mexInfo));
	return m;
}

///Funzioni verso File

int overrideHeadF(convInfo *cI, FILE *stream) {

	rewind(stream);
	if (fWriteF(stream, sizeof(convInfo), 1, cI))
	{
		perror("fwrite fail in overrideHeadF");
		return -1;
	}
	return 0;
}

int saveNewMexF(mex *m, FILE *stream)
{
	size_t lenText = strlen(m->text) + 1;

	fseek(stream, 0, SEEK_END); //mi porto alla fine per aggiungere

	if (fWriteF(stream, sizeof(m->info), 1, &m->info))
	{
		return -1;
	}
	if (fWriteF(stream, lenText, 1, m->text))
	{
		return -1;
	}
	return 0;
}

conversation *loadConvF(FILE *stream)
{
	conversation *conv = malloc(sizeof(conversation));
	conv->stream = stream;
	///accedo al file e lo copio in un buffer in blocco
	fflush(stream);

	struct stat streamInfo;
	fstat(_fileno(stream), &streamInfo);
	printf("[loadConvF] fstat executed, size %d\n", streamInfo.st_size);
	if (streamInfo.st_size == 0)
	{
		conv->head.nMex = 0;
		conv->mexList = NULL;
		return conv;
	}
	char *buf = malloc(streamInfo.st_size);
	if (buf == NULL) {
		printf("[loadConvF] buf error\n");
		return NULL;
	}
	printf("[loadConvF] malloc executed\n");

	rewind(stream);
	fReadF(stream, 1, streamInfo.st_size, buf);
	
	printf("[loadConvF] data acquired in buffer\n");
	///accesso eseguito inizio il compattamento dati

	char *dataPoint;

	///copio i dati di testa

	memcpy(&conv->head, buf, sizeof(conv->head));
	dataPoint = buf + sizeof(conv->head);
	if (streamInfo.st_size == sizeof(conv->head))
	{
		//non sono presenti messaggi e ho una conversazione vuota
		printf("File con solo testa\n");
		free(buf);
		conv->mexList = NULL;
		return conv;
	}

	conv->mexList = malloc(conv->head.nMex * sizeof(mex *));   //creo un array di puntatori a mex
	mex *mexNode;
	size_t len;
	for (int i = 0; i < conv->head.nMex; i++)
	{
		printf("[loadConvF] makeMexlist iteration n. %d\n",i);
		///genero un nuovo nodo dei messaggi in ram
		mexNode = malloc(sizeof(mex));

		///Copio i metadati
		memcpy(mexNode, dataPoint, sizeof(mexInfo));
		dataPoint += sizeof(mexInfo);
		///Crea in ram la stringa di dim arbitraria e mex la punta
		len = strlen(dataPoint) + 1;
		mexNode->text = malloc(len);
		strcpy(mexNode->text, dataPoint);
		dataPoint += len;

		conv->mexList[i] = mexNode;   //salvo il puntatore nell'array
		/*
		printf("\nil nuovo messaggio creato è:\n");
		printMex(mexNode);
		*/
	}
	free(buf);
	return conv;
}

///Funzioni di supporto

int fWriteF(FILE *f, size_t sizeElem, int nelem, char *dat) {
	fflush(f);   /// NECESSARIO SE I USA LA MODALITA +, serve a garantire la sincronia tra R/W
	size_t cont = 0;
	while (cont != sizeElem * nelem) {
		if (ferror(f) != 0)    // testo solo per errori perchè in scrittura l'endOfFile Cresce
		{
			// è presente un errore in scrittura
			errno = EBADF;   //file descriptor in bad state
			return -1;
		}
		//dprintf(fdOut,"prima fwrite; dat=%p\n",dat);
		cont += fwrite(dat + cont, 1, sizeElem * nelem - cont, f);
	}
	return 0;
}

int fReadF(FILE *f, size_t sizeElem, int nelem, char *save) {
	fflush(f);   /// NECESSARIO SE I USA LA MODALITA +, serve a garantire la sincronia tra R/W
	size_t cont = 0;
	while (cont != sizeElem * nelem) {
		if (ferror(f) != 0)    // testo solo per errori perchè in scrittura l'endOfFile Cresce
		{
			printf("[fReadF][ferror] read take error\n");
			// è presente un errore in lettura
			errno = EBADF;   //file descriptor in bad state
			return -1;
		}
		if (feof(f) != 0) {
			printf("[fReadF][feof] read take error\n");
			// è presente un errore in lettura
			errno = EBADF;   //file descriptor in bad state
			return -1;
		}
		cont += fread(save + cont, 1, sizeElem * nelem - cont, f);
		printf("[fReadF][conv] cont %d,  sizeElem * nelem = %d\n", cont, sizeElem * nelem);
	}
	return 0;
}

int freeMex(mex *m) {
	if (m->text) free(m->text);
	if (m->text) free(m);
	return 0;
}

time_t currTimeSys() {
	time_t current_time;

	/* Obtain current time. */
	current_time = time(NULL);

	if (current_time == ((time_t)-1)) {
		fprintf(stderr, "Failure to obtain the current time.\n");
	}
	return current_time;
}

///Funzioni di visualizzazione

void printConv(conversation *c, int fdOutP) {
	printf ( "-------------------------------------------------------------\n");
	printf ( "\tLa Conversazione ha salvati i seguenti messaggi:\n");
	printf ( "\tsizeof(mex)=%ld\tsizeof(mexInfo)=%ld\tsizeof(convInfo)=%ld\n", sizeof(mex), sizeof(mexInfo),
		sizeof(convInfo));
	printf ( "FILE stream pointer\t-> %p\n", c->stream);
	printf ( "\n\t[][]La Conversazione è:[][]\n\n");
	printConvInfo(&c->head, fdOutP);

	//mex *currMex=c->mexList;
	printf ( "##########\n\n");

	for (int i = 0; i < c->head.nMex; i++) {
		printf ( "--->Mex[%d]:\n", i);
		printMex(c->mexList[i], fdOutP);
		printf ( "**********\n");
	}
	printf ( "-------------------------------------------------------------\n");
	//return;
}

void printMex(mex *m, int fdOutP) {
	/*
	m->text
	m->info.usId
	m->info.timeM
	m->next
	 */
	printf ( "Mex data Store locate in=%p:\n", m);
	printf ( "info.usId\t-> %d\n", m->info.usId);
	printf ( "time Message\t-> %s", timeString(m->info.timeM));
	if (m->text != NULL) {
		printf ( "Text:\n-->  %s\n", m->text);
	}
	else {
		printf ( "Text: ##Non Presente##\n");
	}
}

void printMexBuf(char *buf, int fdOutP) {
	/*
	m->text
	m->info.usId
	m->info.timeM
	m->next
	 */
	mex m;
	memcpy(&m, buf, sizeof(mexInfo));
	m.text = buf + sizeof(mexInfo);

	printf ( "Mex buf data Store locate in=%s:\n", buf);
	printf ( "info.usId\t-> %d\n", m.info.usId);
	printf ( "time Message\t-> %s", timeString(m.info.timeM));
	if (m.text != NULL) {
		printf ( "Text:\n-->  %s\n", m.text);
	}
	else {
		printf ( "Text: ##Non Presente##\n");
	}
}

void printConvInfo(convInfo *cI, int fdOutP) {
	/*
	cI->nMex
	cI->adminId
	cI->timeCreate
	*/
	printf ( "#1\tConversation info data Store:\n");
	printf ( "nMess\t\t-> %d\n", cI->nMex);
	printf ( "adminId\t\t-> %d\n", cI->adminId);
	printf ( "Time Creat\t-> %s\n", timeString(cI->timeCreate));
}


char *timeString(time_t t) {
	char *c_time_string;
	/* Convert to local time format. */
		//dprintf(fdOut,"ctime before\n");
	c_time_string = ctime(&t);   /// è thread safe, ha una memoria interna
	//dprintf(fdOut,"ctime after\n");
	if (c_time_string == NULL) {
		fprintf(stderr, "Failure to convert the current time.\n");
	}
	return c_time_string;

}