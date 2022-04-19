#include "m_file.h"

// <>


// return my_error("Le tableau est plein (envoi en mode non bloquant).\n", file, NO_UNLOCK, 'n', EAGAIN);

int my_error(char *txt, MESSAGE *file, bool unlock, char signal, int error){
	perror(txt);
	if(unlock){
		if(pthread_mutex_unlock(&file->shared_memory->head.mutex) != 0){ perror("UNlock mutex"); exit(-1); }
	}
	if(signal=='r' || signal == 'b'){
		if(pthread_cond_signal(&file->shared_memory->head.rcond) > 0){perror("signal rcond"); exit(-1);}
	}
	if(signal=='w' || signal == 'b'){
		if(pthread_cond_signal(&file->shared_memory->head.wcond) > 0){perror("signal wcond"); exit(-1);}
	}
	if(error > 0) { errno = error; }
	exit(-1);
}

// Debug : my_error a utiliser
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
	memcpy(file->shared_memory->messages[position].mtext, msg, len);
	//memcpy(file->shared_memory->messages[position].type, (mon_message*) msg->type, len);

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
	int msg_number;
	bool msg_to_read = false;

	while(!msg_to_read){
		// Verifie s'il y a un message dans la file
		if(type==0 && file->shared_memory->head.first != -1){
			msg_to_read = true;
			msg_number = file->shared_memory->head.first;
		}
		// Verifie s'il y a un message correspondant a 'type' dans la file
		else if(type != 0 && file->shared_memory->head.first != -1){
			int pipe_capacity = file->shared_memory->head.pipe_capacity;
			int first =file->shared_memory->head.first;
			int last = file->shared_memory->head.last;
			for(int i = first; i != last; i = (i+1) % pipe_capacity){
				// S'il y a un message à la case i
				if(file->shared_memory->messages[i].type != -1){
					if((type>0 && file->shared_memory->messages[i].type == type)
							|| (type<0 && file->shared_memory->messages[i].type <= -type)){
						msg_to_read = true;
						msg_number = i;
						break;
					}
				}
			}
		}
		// Exit si pas de message et mode non bloquant
		if(flags == O_NONBLOCK && !msg_to_read){
			/*
			// Determine si on fait un signal sur wcond et/ou rcond
			char signal = 'n';
			if(file->shared_memory->head.first != file->shared_memory->head.last){signal = 'w';}
			if(file->shared_memory->head.first != -1){signal = (signal == 'w') ? 'b' : 'r';}
			*/
			return my_error("Pas de message \'type\' (mode non bloquant).\n", file, UNLOCK, 'b', EAGAIN);
		}
		// Attente si pas de message et mode bloquant
		else if(!msg_to_read){
			if(pthread_cond_wait(&file->shared_memory->head.rcond, &file->shared_memory->head.mutex) > 0) {
				perror("wait wcond"); exit(-1);
			}
		}
	}
	msg_size = file->shared_memory->messages[msg_number].length;
	if(msg_size > len){
		/*
		// Determine si on fait un signal sur wcond et/ou rcond
		char signal = 'n';
		if(file->shared_memory->head.first != file->shared_memory->head.last){signal = 'w';}
		if(file->shared_memory->head.first != -1){signal = (signal == 'w') ? 'b' : 'r';}
		*/
		return my_error("Memoire allouee trop petite pour recevoir le message.", file, UNLOCK, 'b', EMSGSIZE);
	}
	// Modifie first si le message lu est le premier de la file
	if(msg_number == file->shared_memory->head.first){
		// Incremente first tant que pas de message (case vide) et qu'on n'est pas a 'last'
		int first = (msg_number + 1) % file->shared_memory->head.pipe_capacity;
		while(first != file->shared_memory->head.last && file->shared_memory->messages[first].type == -1){
			first = (first + 1) % file->shared_memory->head.pipe_capacity;
		}
		if(file->shared_memory->head.first == file->shared_memory->head.last){
			file->shared_memory->head.first = -1;
		}
		else { file->shared_memory->head.first = first; }
	}
	// Copie et "suppression" du message
	memcpy(msg, file->shared_memory->messages[msg_number].mtext, msg_size);
	file->shared_memory->messages[msg_number].type = -1;

	// Synchronise la memoire
	if(msync(file, sizeof(MESSAGE), MS_SYNC) == -1) {perror("Function msync()"); exit(-1);}

	// Unlock le mutex
	if(pthread_mutex_unlock(&file->shared_memory->head.mutex) != 0){ perror("UNlock mutex"); exit(-1); }

	// Signale des processus attendant de pouvoir envoyer ou recevoir
	if(pthread_cond_signal(&file->shared_memory->head.wcond) > 0){perror("signal wcond"); exit(-1);}
	if(pthread_cond_signal(&file->shared_memory->head.rcond) > 0){perror("signal rcond"); exit(-1);}

	return msg_size;
}
