#ifndef M_FILE_H_
#define M_FILE_H_

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

// STRUCTURES
typedef struct MESSAGE{
	typedef enum{ O_RDWR,  O_RDONLY, O_WRONLY, O_CREAT, O_EXCL } flag;
	void* memory;
} MESSAGE;

typedef struct mon_message{
	long type;
	char mtext[];
} mon_message;

typedef struct header{
	int max_length_message;
	int pipe_capacity;
	int first;
	int last;
	//semaphore ou mutex
} header;

/*
typedef struct pipe{
	struct header head;
	mon_message messages[]; // flexible array members are invalid
} pipe;
*/

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
