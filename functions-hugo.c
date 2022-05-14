#include "m_file.h"

// Renvoie le bit numero i d'un int
int BitAt(long unsigned int x, int i){
	assert(0 <= i && i < 32);
	return (x >> i) & 1;
}

int build_prot(int options){
	int prot = 0;

	if(is_o_rdwr(options)){
		prot = PROT_READ | PROT_WRITE;
	}
	else if(is_o_rdonly(options)){
		prot = PROT_READ;
	}
	else if(is_o_wronly(options)){
		prot = PROT_WRITE;
	}

	if(is_o_excl(options)){
		prot = prot | PROT_EXEC;

	}
	return prot;
}

int is_o_creat(int options){
	int k=0;
	int creat = O_CREAT;
	while(creat!=0){
		k++;
		creat>>=1;
	}
	k--;
	return (options>>k)&1;
}

int is_o_rdonly(int options){
	if( is_o_rdwr(options) ){
		return 0;
	}
	return !(options&1);
}

int is_o_wronly(int options){
	if( is_o_rdwr(options) ){
		return 0;
	}
	return options&1;
}

int is_o_rdwr(int options){
	return (options>>1)&1;
}

int is_o_excl(int options){
	int k=0;
	int excl = O_EXCL;
	while(excl!=0){
		k++;
		excl>>=1;
	}
	k--;
	return (options>>k)&1;
}

int anon_and_shared(const char *nom){
	if(nom==NULL){
		return MAP_SHARED | MAP_ANON;
	}
	return MAP_SHARED;
}

// nom doit commencer par un unique /
//size_t nb_msg, size_t len_max, mode_t mode

void build_msg(MESSAGE* msg, line *addr, const char *nom, int options, size_t nb_msg, size_t len_max, mode_t mode){

	msg->memory_size = sizeof(header) + nb_msg * (len_max * sizeof(char) + sizeof(mon_message)); // debug : memory_size - sizeof(header)
	msg->flag = options;

	int fd = shm_open(nom, options, mode);
	if( fd == -1 ){ perror("shm_open"); exit(1);}
	if( ftruncate(fd, msg->memory_size) == -1 ){perror("ftruncate"); exit(1);}
	struct stat bufStat;
	fstat(fd, &bufStat);

	int prot = build_prot(options);

	int map_flag = anon_and_shared(nom);
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

	// La place disponible pour le premier message
	((mon_message *)&msg->shared_memory->messages[0])->length = msg->memory_size - sizeof(header) - sizeof(mon_message);
	((mon_message *)&msg->shared_memory->messages[0])->offset = 0;

	// Initialisation du mutex et des conditions
	if(initialiser_mutex(&addr->head.mutex) > 0){ perror("init mutex"); exit(1); }
	if(initialiser_cond( &addr->head.rcond ) > 0){ perror("init mutex"); exit(1); }
	if(initialiser_cond( &addr->head.wcond ) > 0){ perror("init mutex"); exit(1); }

	// Initialisations des tableaux pour les notifications
	for(int i = 0; i < RECORD_NB; i++){ msg->shared_memory->head.records[i].pid = -1; }

	for(int i = 0; i < TYPE_SEARCH_NB; i++){
		msg->shared_memory->head.types_searched[i].number = 0;
		msg->shared_memory->head.types_searched[i].type = 0;
	}

}

int connex_msg(MESSAGE *msg, line *addr, const char *nom, int options){

	if(is_o_wronly(options)){ options = O_RDWR; }

	int fd = shm_open(nom, options, 0);
	if( fd == -1 && errno == EEXIST) { printf("La file existe deja et drapeau O_EXCL.\n"); return -1; }
	if( fd == -1 && errno == ENOENT) { printf("La file n'existe pas et pas de drapeau O_CREAT.\n"); return -1; }
	else if( fd == -1 ){ perror("shm_open dans connex_msg"); exit(EXIT_FAILURE); }

	struct stat bufStat;
	fstat(fd, &bufStat);

	int prot = build_prot(options);
	int map_flag = anon_and_shared(nom);
	addr = mmap(NULL, bufStat.st_size, prot, map_flag, fd, 0);
	if( (void*) addr == MAP_FAILED) { perror("Function mmap()"); exit(EXIT_FAILURE); }

	msg->memory_size = sizeof(header) + addr->head.pipe_capacity * (addr->head.max_length_message * sizeof(char) + sizeof(mon_message));
	msg->flag = options;
	msg->shared_memory = addr;
	return 0;
}

int file_exists (const char * f){
    struct stat buff;
    if (stat(f, &buff) != 0) return 0;
    return S_ISREG(buff.st_mode);
}

MESSAGE *m_connexion(const char *nom, int options, ...){

	MESSAGE *msg = malloc(sizeof(MESSAGE));
	line *addr = NULL;

    if(is_o_creat(options)){ //il faut creer la file si elle n'existe pas

		// on empeche de creer une file en lecture seule
    	if(is_o_rdonly(options)){ return NULL; }

    	va_list parametersInfos;
    	va_start(parametersInfos, options);

    	size_t nb_msg = va_arg(parametersInfos, size_t);
    	size_t len_max = va_arg(parametersInfos, size_t);
    	mode_t mode = va_arg(parametersInfos, mode_t);
		
    	if(file_exists(nom)){
    		if(connex_msg(msg, addr, nom, options) == -1) { return NULL; }
    	}else{
	    	build_msg(msg, addr, nom, options, nb_msg, len_max, mode);
    	}


		va_end(parametersInfos);
    }
    else if(!is_o_creat(options) && nom != NULL){ // si la file existe
    	if(connex_msg(msg, addr, nom, options) == -1) { return NULL; }
    }
    else{ return NULL; }

	return msg;
}


int m_deconnexion(MESSAGE *file){
	line *addr = file->shared_memory;
	file->shared_memory = NULL;
	return munmap(addr, file->memory_size);
}

// Est-ce qu'il ne faut pas d'abord appeler m_deconnexion ? (de telle sorte que ca marche meme si on est deja deconnecte)
int m_destruction(const char *nom){
	return shm_unlink(nom);
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
