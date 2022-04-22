#ifndef M_FILE_H_
#define M_FILE_H_

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <pthread.h>
#include <sys/mman.h>
#include <assert.h>
#include <stddef.h>
#include <stdarg.h> // pour avoir le nombre d'arguments qui varie dans une même fonction 

#define UNLOCK true
#define NO_UNLOCK false

#define LEN_NAME 32

// STRUCTURES
typedef struct mon_message{
	long type;
	size_t length;
	ptrdiff_t offset;
	char mtext[];
} mon_message;

typedef struct header{
	size_t max_length_message;
	int pipe_capacity;
	int first_occupied;
	int last_occupied;
	int first_free;
	int last_free;
	pthread_mutex_t mutex;
	pthread_cond_t rcond;
	pthread_cond_t wcond;
} header;

typedef struct line{
	struct header head;
	char messages[];
} line;

typedef struct MESSAGE{
	char name[LEN_NAME]; // on le met dans le doute
	size_t memory_size;
	int flag;
	line* shared_memory;
} MESSAGE;


// PROJECT FUNCTIONS
MESSAGE *m_connexion( const char *nom, int options, ...);
int m_deconnexion(MESSAGE *file);
int m_destruction(const char *nom);

int m_envoi(MESSAGE *file, const void *msg, size_t len, int msgflag);
ssize_t m_reception(MESSAGE *file, void *msg, size_t len, long type, int flags);

size_t m_message_len(MESSAGE *);
size_t m_capacite(MESSAGE *);
size_t m_nb(MESSAGE *);


// ADDITIONNAL FUNCTIONS
int initialiser_mutex(pthread_mutex_t *pmutex);
int initialiser_cond(pthread_cond_t *pcond);
int my_error(char *txt, MESSAGE *file, bool unlock, char signal, int error);
int m_envoi_erreurs(MESSAGE *file, const void *msg, size_t len, int msgflag);


#endif /* M_FILE_H_ */
