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



ssize_t m_reception(MESSAGE *file, void *msg, size_t len, long type, int flags){
	if(file->flag == O_WRONLY){
		printf("Impossible de lire les message de cette file.\n");
		errno = EPERM;
		exit(-1);
	}

	// Lock du mutex
	if(pthread_mutex_lock(&file->shared_memory->head.mutex) != 0){ perror("lock mutex"); exit(-1); }

	ssize_t msg_size;
	int msgNumber;
	bool msgToRead = false;

	while(!msgToRead){
		// Verifie s'il y a un message dans la file
		if(type==0 && file->shared_memory->head.first != -1){
			msgToRead = true;
			msgNumber = file->shared_memory->head.first;
		}
		// Verifie s'il y a un message correspondant a type dans la file
		else if(type != 0 && file->shared_memory->head.first != -1){
			int pipe_capacity = file->shared_memory->head.pipe_capacity;
			int first =file->shared_memory->head.first;
			int last = file->shared_memory->head.last;
			for(int i = first; i != last; i = (i+1) % pipe_capacity){
				if((type>0 && file->shared_memory->messages[i].type == type)
						|| (type<0 && file->shared_memory->messages[i].type <= -type)){
					msgToRead = true;
					msgNumber = i;
					break;
				}
			}
		}
		// Exit si pas de message et mode non bloquant
		if(flags == O_NONBLOCK && !msgToRead){
			if(pthread_mutex_unlock(&file->shared_memory->head.mutex) != 0){ perror("UNlock mutex"); exit(-1); }
			if(file->shared_memory->head.first != -1){
				if(pthread_cond_signal(&file->shared_memory->head.rcond) > 0){perror("signal rcond"); exit(-1);}
			}
			if(pthread_cond_signal(&file->shared_memory->head.wcond) > 0){perror("signal rcond"); exit(-1);}
			printf("Pas de message correspondant a type (reception en mode non bloquant).\n");
			errno = EAGAIN;
			exit(-1);
		}
		// Attente si pas de message et mode bloquant
		else if(!msgToRead){
			if(pthread_cond_wait(&file->shared_memory->head.rcond, &file->shared_memory->head.mutex) > 0) {
				perror("wait wcond"); exit(-1);
			}
		}
	}
	// Copie et suppression du message
	msg_size = file->shared_memory->messages[msgNumber].size;
	msg = &file->shared_memory->messages[msgNumber];
	// /!\ Est-ce que ca marche ? Le tableau n'est-il pas cense contenir des pointeurs (donc il ne devrait pas y avoir besoin de &) ?
	//file->shared_memory->messages[msgNumber]=

	return msg_size;
}




