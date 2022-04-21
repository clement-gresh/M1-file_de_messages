#include "m_file.h"

/*
#define S_IRWXU 0000700     RWX mask for owner
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

int build_prot(int options){
	int options_i=0;
	int prot = 0;
	size_t len_int = sizeof(int);

	for(int i=0;i<len_int;i++){
		options_i = BitAt(options_i,i);
		if((options_i==BitAt(O_WRONLY,i))==1){
			//opt_wr = 1;
			prot= prot | PROT_WRITE;
		}
		if((options_i==BitAt(O_RDONLY,i))==1){
			//opt_rd = 1;
			prot = prot | PROT_READ;
		}
		if((options_i==BitAt(O_RDWR,i))==1){
			//opt_rdwr = 1;
			prot = prot | PROT_READ | PROT_WRITE;
		}
		if((options_i==BitAt(O_EXCL,i))==1){
			//opt_excl = 1;
			prot = prot | PROT_EXEC;
		}
		if((options_i==BitAt(O_CREAT,i))==1){
			//opt_creat = 1;
		}
	}

	return prot;
}

int is_o_creat(int options){
	int options_i=0;
	size_t len_int = sizeof(int);

	for(int i=0;i<len_int;i++){
		if((options_i==BitAt(O_CREAT,i))==1){
			return 1;
		}
	}
	return 0;
}

int private_or_shared(const char *nom){
	if(nom==NULL){
		return MAP_PRIVATE;
	}
	return MAP_SHARED;
}

/*MESSAGE *m_connexion(const char *nom, int options, size_t nb_msg, size_t len_max, mode_t mode){
	// ne verifie pas le partage de memoire Anonyme

	MESSAGE *msg = malloc(sizeof(MESSAGE));

	size_t len_name = sizeof(nom); // debug : est ce que le champ nom est utile ?
	assert(len_name<LEN_NAME);

	int fd = shm_open(nom, options, mode);
	if( fd == -1 ){ perror("shm_open"); exit(1);}
	if( ftruncate(fd, msg->memory_size) == -1 ){perror("ftruncate"); exit(1);}
	struct stat bufStat;
	fstat(fd, &bufStat);

	int prot = build_prot(options);
	int map_flag = private_or_shared(nom);
	line *addr = mmap(NULL, bufStat.st_size, prot, map_flag, fd, 0);
	if( (void*) addr == MAP_FAILED) {
		perror("Function mmap()");
		exit(EXIT_FAILURE);
	}

	memcpy(msg->name, nom, len_name);
	msg->memory_size = sizeof(header) + nb_msg * (len_max * sizeof(char) + sizeof(mon_message));
	msg->flag = options;
	msg->shared_memory = addr;

	msg->shared_memory->head.max_length_message = len_max;
	msg->shared_memory->head.pipe_capacity = nb_msg;
	msg->shared_memory->head.first_occupied = -1;
	msg->shared_memory->head.last_occupied = -1;
	msg->shared_memory->head.first_free = 0;
	msg->shared_memory->head.last_free = 0;

	// La place disponible pour un message
	((mon_message*)msg->shared_memory->messages)[0].length = msg->memory_size;
	((mon_message*)msg->shared_memory->messages)[0].offset = 0;

	if(initialiser_mutex(&addr->head.mutex) > 0){ perror("init mutex"); exit(1); }
	if(initialiser_cond( &addr->head.rcond ) > 0){ perror("init mutex"); exit(1); }
	if(initialiser_cond( &addr->head.wcond ) > 0){ perror("init mutex"); exit(1); }

	return msg;
}*/

// nom doit commencer par un unique /
//size_t nb_msg, size_t len_max, mode_t mode
MESSAGE *m_connexion(const char *nom, int options, ...){

	MESSAGE *msg = malloc(sizeof(MESSAGE));

	size_t len_name = sizeof(nom); // debug : est ce que le champ nom est utile ?
	assert(len_name<LEN_NAME);

	line *addr = NULL;

    if(is_o_creat(options)){ //il faut le creer s'il n'existe pas
    	va_list parametersInfos;
    	va_start(parametersInfos, options);

    	size_t nb_msg = va_arg(parametersInfos, size_t);
    	size_t len_max = va_arg(parametersInfos, size_t);
    	mode_t mode = va_arg(parametersInfos, mode_t);

    	memcpy(msg->name, nom, len_name);
		msg->memory_size = sizeof(header) + nb_msg * (len_max * sizeof(char) + sizeof(mon_message));
		msg->flag = options;

		int fd = shm_open(nom, options, mode);
		if( fd == -1 ){ perror("shm_open"); exit(1);}
		if( ftruncate(fd, msg->memory_size) == -1 ){perror("ftruncate"); exit(1);}
		struct stat bufStat;
		fstat(fd, &bufStat);
		printf("ici _n\n");
		int prot = build_prot(options);
		int map_flag = private_or_shared(nom);
		addr = mmap(NULL, bufStat.st_size, prot, map_flag, fd, 0);
		if( (void*) addr == MAP_FAILED) {
			perror("Function mmap()");
			exit(EXIT_FAILURE);
		}
	
		msg->shared_memory = addr;
		msg->shared_memory->head.max_length_message = len_max;
		msg->shared_memory->head.pipe_capacity = nb_msg;
		msg->shared_memory->head.first_occupied = -1;
		msg->shared_memory->head.last_occupied = -1;
		msg->shared_memory->head.first_free = 0;
		msg->shared_memory->head.last_free = 0;

		// La place disponible pour un message
		((mon_message*)msg->shared_memory->messages)[0].length = msg->memory_size;
		((mon_message*)msg->shared_memory->messages)[0].offset = 0;

		va_end(parametersInfos);

    }
    else if(!is_o_creat(options) && nom!=NULL){ // il existe
    	MESSAGE *msg = malloc(sizeof(MESSAGE));

		size_t len_name = sizeof(nom); // debug : est ce que le champ nom est utile ?
		assert(len_name<LEN_NAME);

		int fd = shm_open(nom, options, 0);
		if( fd == -1 ){ perror("shm_open"); exit(1);}
		if( ftruncate(fd, msg->memory_size) == -1 ){perror("ftruncate"); exit(1);}
		struct stat bufStat;
		fstat(fd, &bufStat);

		int prot = build_prot(options);
		addr = mmap(NULL, bufStat.st_size, prot, MAP_SHARED, fd, 0);
		if( (void*) addr == MAP_FAILED) {
			perror("Function mmap()");
			exit(EXIT_FAILURE);
		}

		memcpy(msg->name, nom, len_name);
		msg->memory_size = sizeof(addr->head) + addr->head.pipe_capacity * (addr->head.max_length_message * sizeof(char) + sizeof(mon_message));
		msg->flag = options;
		msg->shared_memory = addr;

    }
    else{
    	return NULL;
    }

	if(initialiser_mutex(&addr->head.mutex) > 0){ perror("init mutex"); exit(1); }
	if(initialiser_cond( &addr->head.rcond ) > 0){ perror("init mutex"); exit(1); }
	if(initialiser_cond( &addr->head.wcond ) > 0){ perror("init mutex"); exit(1); }

	return msg;

}


int m_deconnexion(MESSAGE *file){
	file->shared_memory = NULL;
	return 0;
}

int m_destruction(const char *nom){
	int fd = shm_open(nom, O_RDONLY,0);

	struct stat bufStat ;
	/*int rsStat =*/ fstat(fd, &bufStat);
	int len = bufStat.st_size;

	line *srcmem = mmap(0, len , PROT_READ , MAP_PRIVATE | MAP_SHARED, fd, 0);

	munmap(srcmem, len);

	return 0;
}

int initialiser_mutex(pthread_mutex_t *pmutex){
	pthread_mutexattr_t mutexattr;
	int code;
	if( ( code = pthread_mutexattr_init(&mutexattr) ) != 0)
		return code;
	if( ( code = pthread_mutexattr_setpshared(&mutexattr, PTHREAD_PROCESS_SHARED) ) != 0)
		return code;
	code = pthread_mutex_init(pmutex, &mutexattr) ;
	return code;
}

int initialiser_cond(pthread_cond_t *pcond){
	pthread_condattr_t condattr;
	int code;
	if( ( code = pthread_condattr_init(&condattr) ) != 0 ) return code;
	if( ( code = pthread_condattr_setpshared(&condattr, PTHREAD_PROCESS_SHARED) ) != 0 )
		return code;
	code = pthread_cond_init(pcond, &condattr) ;
	return code;
}
