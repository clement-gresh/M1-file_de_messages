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
#include <math.h> // pour les tests

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
int my_error(char *txt, MESSAGE *file, bool unlock, char signal, int error);
int m_envoi_erreurs(MESSAGE *file, size_t len, int msgflag);
void m_envoi_libres(MESSAGE *file, int current, size_t len);
void m_envoi_occupees(MESSAGE *file, int current);
int m_envoi_recherche(MESSAGE *file, size_t len, int msgflag);
int m_reception_erreurs(MESSAGE *file, int flags);
void m_reception_occupees(MESSAGE *file, int current);
void m_reception_libres(MESSAGE *file, int current);
int m_reception_recherche(MESSAGE *file, long type, int flags);
int is_o_creat(int options);
int private_or_shared(const char *nom);
int build_prot(int options);
int BitAt(long unsigned int x, int i);


// FONCTIONS DE TEST
int reception_check(mon_message *m1, char text[], ssize_t s, ssize_t size, size_t length, int value2, long type);
int index_check(header * head, char text[], int first_free, int last_free, int first_occupied, int last_occupied);
int offset_check(mon_message * messages, char text[], int offset, int position);
int error_check(char text[], ssize_t s, int error);
int test_connexion();
int test_envoi_erreurs();
int test_envoi(MESSAGE* file);
int test_envois_multiples(MESSAGE* file, int msg_nb);
int test_reception_erreurs();
int test_reception(MESSAGE* file);
int test_receptions_multiples_fin(header *head, mon_message *messages, int msg_nb, size_t size_msg,
		int position1, int position2, int position3);
int test_reception_type_pos(MESSAGE* file, mon_message *m1, size_t size_msg, int msg_nb, int position1);
int test_reception_type_neg(MESSAGE* file, mon_message *m1, size_t size_msg, int msg_nb, int position1, int position2);
int test_receptions_multiples(MESSAGE* file, int msg_nb);


#endif /* M_FILE_H_ */
