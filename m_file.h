#ifndef M_FILE_H_
#define M_FILE_H_

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

// STRUCTURES
typedef struct mon_message{
	long type;
	const char* mtext;
} mon_message;

typedef struct header{
	int max_length_message;
	int pipe_capacity;
	int first;
	int last;
	pthread_mutex_t mutex;//semaphore ou mutex
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


// FUNCTIONS
MESSAGE *m_connexion( const char *nom, int options, size_t nb_msg, size_t len_max, mode_t mode );
int m_deconnexion(MESSAGE *file);
int m_destruction(const char *nom);

int m_envoi(MESSAGE *file, const void *msg, size_t len, int msgflag);
ssize_t m_reception(MESSAGE *file, void *msg, size_t len, long type, int flags);

size_t m_message_len(MESSAGE *);
size_t m_capacite(MESSAGE *);
size_t m_nb(MESSAGE *);

#endif /* M_FILE_H_ */
