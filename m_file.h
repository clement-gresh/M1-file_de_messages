#ifndef M_FILE_H_
#define M_FILE_H_

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <pthread.h>
#include <sys/mman.h>
#include <assert.h>

// STRUCTURES
typedef struct mon_message{
	long type;
	char* mtext;
} mon_message;

typedef struct header{
	int max_length_message;
	int pipe_capacity;
	int first;
	int last;
	pthread_mutex_t mutex;//semaphore ou mutex
	pthread_cond_t rcond;
	pthread_cond_t wcond;
} header;

typedef struct line{
	struct header head;
	mon_message *messages;
} line;

typedef struct MESSAGE{
	char name; // on le met dans le doute
	int flag;
	line* shared_memory;
} MESSAGE;


// PROJECT FUNCTIONS
MESSAGE *m_connexion( const char *nom, int options, size_t nb_msg, size_t len_max, mode_t mode );
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
int m_envoi_erreurs(MESSAGE *file, const void *msg, size_t len, int msgflag);


#endif /* M_FILE_H_ */
