//
// Created by alfylinux on 20/09/18.
//

#include "tableFile.h"

/// Funzioni di Interfaccia operanti su Tabella

/** La funzione ha lo scopo di creare una tabella completamente nuova in memoria **/

table *open_Tab(char * path)
{
	FILE *f;
	f = openTabF(path);
	return makeTable(f);
}

int addEntry(table *t, char *name, int data)
{
	if (addEntryTabF(t->stream, name, data))
	{
		//Aggiunta non avvenuta con successo
		return -1;
	}
	firstFree *first = &t->head;
	entry *freeData = &t->data[t->head.nf_id];
	if (isLastEntry(freeData))		//se è la fine si cambiano i valori e si crea un nuovo last-entry
	{
		/// last free diventa un dato
		freeData->point = data;
		strncpy(freeData->name, name, nameEntrySize);
		///first-free viene cambiato il luo next-free
		first->nf_id++;
		first->len++;
		first->counter = 1;
		/// viene generato un nuovo last-free a fine file
		/// setup di last
		t->data = reallocarray(t->data, first->len, sizeof(entry));
		entry *last = &t->data[t->head.len - 1];
		memset(last->name, 0, nameEntrySize);
		last->point = -1;
	}
	else		//si trasforma in una cella dati e first-free punta la successiva

	{
		first->nf_id = freeData->point;   //first free ora punta una nuova casella libera
		first->counter--;
		/// la prima casella libera diventa un dato
		freeData->point = data;
		strncpy(freeData->name, name, nameEntrySize);
	}
	return 0;
}

int delEntry(table *t, int index)
{
	if (delEntryTabF(t->stream, index))
	{
		//Eliminazione non avvenuta con successo
		return -1;
	}
	firstFree *first = &t->head;
	entry *delData = &t->data[index];
	if (isEmptyEntry(delData))
	{
		// è già una cella cancellata, e non devo modificare nulla
		printf("la casella è vuota\n");
		return 0;
	}
	delData->name[0] = 0;  //metto la stringa a ""
	delData->point = first->nf_id;
	first->nf_id = index;
	first->counter++;
	return 0;
}

int searchFirstOccurrence(table *t, char *search) {
	for (int i = 0; i < t->head.len; i++) {
		if (strcmp(t->data[i].name, search) == 0) {
			return i;
		}
	}
	return -1;
}

int searchFirstOccurrenceKey(table *t, int ID) {
	for (int i = 0; i < t->head.len; i++) {
		if (strtol(t->data[i].name, NULL, 10) == ID) {
			return i;
		}
	}
	return -1;
}

/// Funzioni di supporto operanti sul file
FILE *openTabF(char* path)
{
	FILE *f = fopen(path, "w+x"); // exclusive access on file, fail if it exists
	if (f == NULL)
	{
		f = fopen(path, "r+");
		printf("[fopen] r+ executed, %p\n", f);
		if (f == NULL) {
			printf("[FOPEN] failed r+ opening\n");
			return NULL;
		}
	}
	return f;
}

int addEntryTabF(FILE *tab, char *name, int data)
{
	/** L'operazione è eseguita in modo atomico rispetto ai Tread del processo **/
	firstFree first;
	entry freeData;

	rewind(tab);        //posiziono il seek all'inizio per leggere la prima entry libera
	fflush(tab);    //garantisco che tutto quello che va scritto venga scritto nel file e quindi letto
	fread(&first, 1, sizeof(firstFree), tab);
	int enSeek = entrySeekF(tab, first.nf_id);
	if (enSeek == -1) {
		perror("in addEntryTabF entrySeekF take error:");
		printf("index richiesto:%d\n", first.nf_id);
		exit(-1);
	}

	fseek(tab, enSeek, SEEK_SET);
	fread(&freeData, 1, sizeof(freeData), tab);

	if (isLastEntry(&freeData))		//se è la fine si cambiano i valori e si crea un nuovo last-entry

	{
		/// last free diventa un dato
		freeData.point = data;
		strncpy(freeData.name, name, nameEntrySize);

		/// viene generato un nuovo last-free a fine file
		/// setup di last
		entry last;
		memset(last.name, 0, nameEntrySize);
		last.point = -1;

		///first-free viene cambiato il luo next-free
		first.nf_id++;
		first.len++;
		first.counter = 1;

		///File write
		fseek(tab, enSeek, SEEK_SET);
		if (fileWrite(tab, sizeof(entry), 1, &freeData))
		{
			perror("Write entry empty take error:");
			return -1;
		}
		if (fileWrite(tab, sizeof(entry), 1, &last))
		{
			perror("Write new-last-entry  take error:");
			return -1;
		}
		fseek(tab, 0, SEEK_SET);
		if (fileWrite(tab, sizeof(firstFree), 1, &first))
		{
			perror("Override FirstFree take error:");
			return -1;
		}

	}
	else 		//si trasforma in una cella dati e first-free punta la successiva
	{
		first.nf_id = freeData.point;   //first free ora punta una nuova casella libera
		first.counter--;
		/// la prima casella libera diventa un dato
		freeData.point = data;
		strncpy(freeData.name, name, nameEntrySize);
		fseek(tab, enSeek, SEEK_SET);
		if (fileWrite(tab, sizeof(entry), 1, &freeData))
		{
			perror("Override entry take error:");
			return -1;
		}
		rewind(tab);
		if (fileWrite(tab, sizeof(firstFree), 1, &first))
		{
			perror("Override FirstFree take error:");
			return -1;
		}
	}
	return 0;
}

int delEntryTabF(FILE *tab, int index)
{
	firstFree first;
	entry delData;


	int enDelSeek = entrySeekF(tab, index);
	if (enDelSeek == -1) {
		perror("in delEntryTabF entrySeekF take error:");
		printf("index richiesto:%d\n", index);
		exit(-1);
	}

	fflush(tab);    //garantisco che tutto quello che va scritto venga scritto
	rewind(tab);        //posiziono il seek all'inizio per leggere la prima entry libera
	fread(&first, 1, sizeof(first), tab);
	fseek(tab, enDelSeek, SEEK_SET);
	fread(&delData, 1, sizeof(delData), tab);

	if (isEmptyEntry(&delData))
	{
		// è già una cella cancellata, e non devo modificare nulla
		return 0;
	}

	delData.name[0] = 0;  //metto la stringa a ""
	delData.point = first.nf_id;
	first.nf_id = index;
	first.counter++;

	fseek(tab, enDelSeek, SEEK_SET);
	if (fileWrite(tab, sizeof(entry), 1, &delData))
	{
		perror("Override entry on delete take error:");
		return -1;
	}
	rewind(tab);
	if (fileWrite(tab, sizeof(firstFree), 1, &first))
	{
		perror("Override FirstFree on delete take error:");
		return -1;
	}
	return 0;
}

int entrySeekF(FILE *tab, int i)
{
	struct stat tabInfo;
	fstat(_fileno(tab), &tabInfo);

	int seek = sizeof(firstFree) + sizeof(entry) *i; // se i=0 fseek viene posizionato al byte successivo a first free
	if (seek > tabInfo.st_size - sizeof(entry)) //se seek va oltre la posizione dell'ultima entry DEVE essere errato
	{
		errno = EINVAL;
		return -1;
	}
#ifdef DEB_STR
	printf("Entry seek:\nsize=%d\nindex=%d\nseek=%d\n", tabInfo.st_size, i, seek);
#endif
	return seek;
}

size_t lenTabF(FILE *tab)
{
	struct stat tabInfo;
	fstat(_fileno(tab), &tabInfo);
	return (tabInfo.st_size - sizeof(firstFree)) / sizeof(entry); // nbyte di tipo entry, diviso la dimensione ritorna il numero di elementi
}

int fileWrite(FILE *f, size_t sizeElem, int nelem, char *dat)
{
	fflush(f);   /// NECESSARIO SE I USA LA MODALITA +, serve a garantire la sincronia tra R/W
	size_t cont = 0;
	while (cont != sizeElem * nelem)
	{
		if (ferror(f) != 0)    // testo solo per errori perchè in scrittura l'endOfFile Cresce
		{
			// è presente un errore in scrittura
			errno = EBADF;   //file descriptor in bad state
			return -1;
		}
		cont += fwrite(dat + cont, 1, sizeElem*nelem - cont, f);
	}
	return 0;
}

///Show funciton
void firstPrint(firstFree *f) {
	printf("#1\tfirstFree data Store:\nname\t\t-> %s\ncouterFree\t-> %d\nLen\t\t-> %d\nnextFree\t-> %d\n", f->name, f->counter, f->len, f->nf_id);
	return;
}

void entryPrint(entry *e) {
	printf("Entry data Store:\n??-Last-Free -> %s\tempty  -> %s\nname\t\t-> %s\npoint\t\t-> %d\n", booleanPrint(isLastEntry(e)), booleanPrint(isEmptyEntry(e)), e->name, e->point);
	return;
}

void tabPrint(table *tab)
{
	struct stat tabInfo;
	fstat(_fileno(tab->stream), &tabInfo);
	if (tabInfo.st_size == 0)
	{
		printf("File Vuoto, o Inesistente\n");
		return;
	}

	size_t lenFile = lenTabF(tab->stream);

	printf("-------------------------------------------------------------\n");
	printf("\tLa tabella ha le seguenti caratteristiche:\n\tsize=%zd\n\tlenFile=%zd\tlenFirst=%zd\n", tabInfo.st_size, lenFile, tab->head.len);
	printf("\tsizeof(entry)=%d\tsizeof(firstFree)=%d\n", sizeof(entry), sizeof(firstFree));
	printf("\n\t[][]La tabella contenuta nel file contiene:[][]\n\n");
	firstPrint(&tab->head);
	printf("##########\n\n");
	for (int i = 0; i < tab->head.len; i++)
	{
		printf("--->entry[%d]:", i);
		entryPrint((&tab->data[i]));
		printf("**********\n");
	}
	printf("-------------------------------------------------------------\n");
	return;
}

///funzioni di supporto operanti in ram


int isLastEntry(entry *e)
{
	if (e->name[0] == 0 && e->point == -1)	return 1;
	return 0;
}

int isEmptyEntry(entry *e)
{
	if (e->name[0] == 0 && e->point != -1)	return 1;
	return 0;
}

char *booleanPrint(int i)
{
	if (i) return "True";
	else return "False";
}

table *makeTable(FILE *tab)
{
	fflush(tab);    //garantisco che tutto quello che va scritto venga scritto
	struct stat tabInfo;
	fstat(_fileno(tab), &tabInfo);
	if (tabInfo.st_size == 0)
	{
		printf("File Vuoto, o Inesistente\n");
		return NULL;
	}

	size_t len = lenTabF(tab);

	table *t = (table *)malloc(sizeof(table));
	if (len == 0) {
		t->data = (entry *)calloc(1, sizeof(entry));
		t->data[0].name[0] = 0;
		t->data[0].point = -1;

		rewind(tab);
		fread(&t->head, 1, sizeof(firstFree), tab);

		t->stream = tab;
		return t;

	}
	else {
		t->data = (entry *)calloc(len, sizeof(entry));

		rewind(tab);
		fread(&t->head, 1, sizeof(firstFree), tab);
		fread(t->data, 1, sizeof(entry) * len, tab);

		t->stream = tab;
		return t;
	}
}

