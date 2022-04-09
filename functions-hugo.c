#include "m_file.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <pthread.h>

/*
#define S_IRWXU 0000700    /* RWX mask for owner 
#define S_IRUSR 0000400     R for owner 
#define S_IWUSR 0000200   W for owner 
#define S_IXUSR 0000100     X for owner 

#define S_IRWXG 0000070    RWX mask for group 
#define S_IRGRP 0000040     R for group 
#define S_IWGRP 0000020     W for group 
#define S_IXGRP 0000010     X for group 

#define S_IRWXO 0000007     RWX mask for other 
#define S_IROTH 0000004     R for other 
#define S_IWOTH 0000002     W for other 
#define S_IXOTH 0000001     X for other 

#define S_ISUID 0004000     set user id on execution 
#define S_ISGID 0002000     set group id on execution 
#define S_ISVTX 0001000     save swapped text even after use

*/

/*
typedef struct mon_message{
	long type;
	char* mtext;
} mon_message;

typedef struct header{
	int max_length_message;
	int pipe_capacity;
	int first;
	int last;
	//semaphore ou mutex
} header;

typedef struct file{
	struct header head;
	mon_message *messages;
} file;

typedef struct MESSAGE{
	const char name; // on le met dans le doute
	int flag;
	file* shared_memory;
} MESSAGE;
*/

// Renvoie le bit numero i d'un int
int BitAt(long unsigned int x, int i){
	assert(0 <= i && i < 32);
	return (x >> i) & 1;
}

// nom commence par un unique /
MESSAGE *m_connexion(const char *nom, int options, size_t nb_msg,size_t len_max, mode_t mode){
	// ne vérifie pas le partage de mémoire Anonyme

	int fd = shm_open(nom, options, mode);
	if( fd == -1 ){ perror("shm_open"); exit(1);}

	mon_message m;
	m.type = 0;
	m.mtext = malloc(sizeof(char)*len_max);

	header h;
	h.max_length_message = len_max;
	h.pipe_capacity = nb_msg;
	h.first = 0;
	h.last = 0;

	file f;
	f.head = h;
	f.messages = malloc(sizeof(m)*nb_msg);

	MESSAGE *msg;
	msg->name = *nom;
	msg->flag = options;
	msg->shared_memory = malloc(sizeof(f));

	if( ftruncate( fd, sizeof(m)) == -1){perror("ftruncate"); exit(1);}
	struct stat bufStat ;
	fstat(fd, &bufStat);

	free(m.mtext);
	free(f.messages);
	free(msg->shared_memory);

	int opt_rd = 0;
	int opt_wr = 0;
	int opt_rdwr = 0;
	int opt_excl = 0;
	int opt_creat = 0;

	int prot = 0;

	int options_i=0;
	size_t len_int = sizeof(int);

	mfor(int i=0;i<len_int;i++){
		options_i = BitAt(options_i,i);
		if((options_i==BitAt(O_WRONLY,i))==1){
			opt_wr = 1;
			prot= prot | PROT_WRITE;
		}
		if((options_i==BitAt(O_RDONLY,i))==1){
			opt_rd = 1;
			prot = prot | PROT_READ;
		}
		if((options_i==BitAt(O_RDWR,i))==1){
			opt_rdwr = 1;
			prot = prot | PROT_READ | PROT_WRITE;
		}
		if((options_i==BitAt(O_EXCL,i))==1){
			opt_excl = 1;
			prot = prot | PROT_EXEC;
		}
		if((options_i==BitAt(O_CREAT,i))==1){
			opt_creat = 1;
		}
	}

	file *adrr = mmap(NULL, bufStat.st_size, prot, MAP_SHARED, fd, 0) ;
	msg->shared_memory = adrr;

	return msg;
}


int main(int argc, const char * argv[]) {

	return EXIT_SUCCESS;
}