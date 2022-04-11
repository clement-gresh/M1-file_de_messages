#include "m_file.h"

// <>

int m_envoi_erreurs(MESSAGE *file, const void *msg, size_t len, int msgflag){
	if(file->flag == O_RDONLY){
		printf("Impossible d'ecrire dans cette file.\n");
		errno = EPERM;
		exit(-1);
	}
	if(len > file->shared_memory->head.max_length_message){
		printf("La taille du message excede la taille maximale autorisee.\n");
		errno = EMSGSIZE;
		exit(-1);
	}
	if(msgflag != 0 && msgflag != O_NONBLOCK){
		printf("Valeur de msgflag incorrecte.\n");
		errno = EIO;
		exit(-1);
	}
	if((file->shared_memory->head.first == file->shared_memory->head.last) && (msgflag == O_NONBLOCK)){
		printf("Le tableau est plein (envoi en mode non bloquant).\n");
		errno = EAGAIN;
		exit(-1);
	}
	return 0;
}

int m_envoi(MESSAGE *file, const void *msg, size_t len, int msgflag){
	// Traitement des erreurs
	if(m_envoi_erreurs(file, msg, len, msgflag) < 0){ exit(-1); }

	// Lock du mutex
	if(pthread_mutex_lock(&file->shared_memory->head.mutex) != 0){ perror("lock mutex"); exit(-1); }

	// Attente si tableau plein (sauf si O_NONBLOCK)
	while((file->shared_memory->head.first == file->shared_memory->head.last)){
		if(msgflag == O_NONBLOCK) {
			if(pthread_mutex_unlock(&file->shared_memory->head.mutex) != 0){ perror("UNlock mutex"); exit(-1); }
			if(pthread_cond_signal(&file->shared_memory->head.rcond) > 0){perror("signal rcond"); exit(-1);}
			printf("Le tableau est plein (envoi en mode non bloquant).\n");
			errno = EAGAIN;
			exit(-1);
		}
		if(pthread_cond_wait(&file->shared_memory->head.wcond, &file->shared_memory->head.mutex) > 0) {
			perror("wait wcond"); exit(-1);
		}
	}

	// Incremente 'last' et met a jour 'first' si besoin
	int position = file->shared_memory->head.last;
	file->shared_memory->head.last = (position + 1) % file->shared_memory->head.pipe_capacity;
	if(file->shared_memory->head.first == -1) { file->shared_memory->head.first = position; }

	// Unlock le mutex
	if(pthread_mutex_unlock(&file->shared_memory->head.mutex) != 0){ perror("UNlock mutex"); exit(-1); }

	// Signale un processus attendant de pouvoir envoyer
	if(pthread_cond_signal(&file->shared_memory->head.wcond) > 0){perror("signal wcond"); exit(-1);}

	// Ecrit le message dans la memoire partagee
	file->shared_memory->messages[position] = *(mon_message*) msg;

	// DEBUG
	printf("La valeur du type est %ld.\n", file->shared_memory->messages[file->shared_memory->head.last].type);
	printf("Le msg est %s.\n", file->shared_memory->messages[file->shared_memory->head.last].mtext);
	// FIN DEBUG

	// Synchronise la memoire
	if(msync(file, sizeof(MESSAGE), MS_SYNC) == -1) {
		perror("Function msync()");
		exit(-1);
	}

	// Signale un processus attendant de pouvoir recevoir
	if(pthread_cond_signal(&file->shared_memory->head.rcond) > 0){perror("signal rcond"); exit(-1);}

	return 0;
}
