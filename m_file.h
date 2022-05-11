#ifndef M_FILE_H_
#define M_FILE_H_

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <pthread.h>
#include <sys/mman.h>
#include <assert.h>
#include <stddef.h>
#include <stdarg.h> // pour avoir le nombre d'arguments qui varie dans une meme fonction
#include <sys/wait.h>

#define UNLOCK true
#define NO_UNLOCK false
#define DECR true
#define NO_DECR false

#define RECORD_NB 5
#define TYPE_SEARCH_NB 100

// STRUCTURES
// Permet de stocker les info utiles quand un processus s'enregistre sur la file
typedef struct record{ // debug : pas FIFO
	pid_t pid;						// pid du processus s'etant enregistre pour recevoir un signal
	int signal;						// signal a envoye au processus quand un message est disponible
	long type;						// type de message voulu par le processus
} record;

// Permet de stocker le type d'un message et le nombre de processus en attente pour ce type
typedef struct type_search{
	long type;						// type de message recherche par un/des processus
	int number;						// nombre de processus en attente pour la reception de ce type de message
} type_search;

typedef struct mon_message{
	long type;
	size_t length;
	ptrdiff_t offset;
	char mtext[];
} mon_message;

typedef struct header{
	size_t max_length_message;		// Taille maximale d'un message
	int pipe_capacity;				// Nombre minimum de messages pouvant etre stockes dans la file
	int first_occupied;				// Indice de la premiere case de la Liste Chainee des cases libres
	int last_occupied;				// Indice de la derniere case de la Liste Chainee des cases libres
	int first_free;					// Indice de la premiere case de la Liste Chainee des cases occupees
	int last_free;					// Indice de la derniere case de la Liste Chainee des cases occupees
	pthread_mutex_t mutex;
	pthread_cond_t rcond;
	pthread_cond_t wcond;
	record records[RECORD_NB];						// Liste des processus enregistres pour recevoir un signal
	type_search types_searched[TYPE_SEARCH_NB];		// Liste des 'types' de messages voulus par les proc. en attente sur la file
} header;

typedef struct line{
	struct header head;
	char messages[];
} line;

typedef struct MESSAGE{
	size_t memory_size;
	int flag;
	line* shared_memory;
} MESSAGE;


// FONCTIONS DU PROJET
MESSAGE *m_connexion( const char *nom, int options, ...);
int m_deconnexion(MESSAGE *file);
int m_destruction(const char *nom);

int m_envoi(MESSAGE *file, const void *msg, size_t len, int msgflag);
ssize_t m_reception(MESSAGE *file, void *msg, size_t len, long type, int flags);

size_t m_message_len(MESSAGE *);
size_t m_capacite(MESSAGE *);
size_t m_nb(MESSAGE *);


// FONCTIONS AUXILIAIRES
int initialiser_mutex(pthread_mutex_t *pmutex);
int initialiser_cond(pthread_cond_t *pcond);
int my_error(char *txt, MESSAGE *file, bool decrease, long type, bool unlock, char signal, int error);
int m_envoi_erreurs(MESSAGE *file, size_t len, int msgflag);
void m_envoi_libres(MESSAGE *file, int current, size_t len);
void m_envoi_occupees(MESSAGE *file, int current);
int m_envoi_recherche(MESSAGE *file, size_t len, int msgflag);
int m_reception_erreurs(MESSAGE *file, int flags);
void m_reception_occupees(MESSAGE *file, int current);
void m_reception_libres(MESSAGE *file, int current);
int m_reception_recherche(MESSAGE *file, long type, int flags);
int is_o_creat(int options);
int is_o_rdonly(int options);
int is_o_wronly(int options);
int is_o_rdwr(int options);
int is_o_excl(int options);
int private_or_shared(const char *nom);
int build_prot(int options);
int BitAt(long unsigned int x, int i);

#endif /* M_FILE_H_ */
