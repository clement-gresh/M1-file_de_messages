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
	if((file->shared_memory->head.first_free == -1) && (msgflag == O_NONBLOCK)){
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
	while((file->shared_memory->head.first_occupied == file->shared_memory->head.first_free)){
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

	// Met a jour 'first_free', 'first_occupied' et 'last_occupied'
	int current = file->shared_memory->head.first_free;
	int offset_free = file->shared_memory->messages[current].offset;
	file->shared_memory->messages[current].offset = 0;

	// Si le tableau est plein une fois l'envoi fait
	if(offset_free == 0){ file->shared_memory->head.first_free = -1; }
	else{ file->shared_memory->head.first_free = current + offset_free; }

	// Si le tableau etait vide avant l'envoi
	if(file->shared_memory->head.first_occupied == -1) { file->shared_memory->head.first_occupied = current; }
	else{
		int last_occupied = file->shared_memory->head.last_occupied;
		file->shared_memory->messages[last_occupied].offset = last_occupied - current;

	}
	file->shared_memory->head.last_occupied = current;

	// Unlock le mutex
	if(pthread_mutex_unlock(&file->shared_memory->head.mutex) != 0){ perror("UNlock mutex"); exit(-1); }

	// Signale un processus attendant de pouvoir envoyer
	if(pthread_cond_signal(&file->shared_memory->head.wcond) > 0){perror("signal wcond"); exit(-1);}

	// Ecrit le message dans la memoire partagee
	memcpy(file->shared_memory->messages[current].mtext, msg, len);

	// DEBUG
	printf("La valeur du type est %ld.\n", file->shared_memory->messages[file->shared_memory->head.first_free].type);
	printf("Le msg est %s.\n", file->shared_memory->messages[file->shared_memory->head.first_free].mtext);
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
	int current;
	bool msg_to_read = false;

	while(!msg_to_read){
		current = file->shared_memory->head.first_occupied;

		// Verifie s'il y a un message correspondant a 'type' dans la file
		if(type==0 && current != -1){
			msg_to_read = true;
		}
		else if(type != 0 && current != -1){
			while(true){
				if((type>0 && file->shared_memory->messages[current].type == type)
						|| (type<0 && file->shared_memory->messages[current].type <= -type)){
					msg_to_read = true;
					break;
				}
				else{
					if(current == file->shared_memory->head.last_occupied) { break; }
					current = current + file->shared_memory->messages[current].offset;
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
	msg_size = file->shared_memory->messages[current].length;
	if(msg_size > len){
		/*
		// Determine si on fait un signal sur wcond et/ou rcond
		char signal = 'n';
		if(file->shared_memory->head.first != file->shared_memory->head.last){signal = 'w';}
		if(file->shared_memory->head.first != -1){signal = (signal == 'w') ? 'b' : 'r';}
		*/
		return my_error("Memoire allouee trop petite pour recevoir le message.", file, UNLOCK, 'b', EMSGSIZE);
	}
	// MAJ de la liste chainee de cases occupee
	int search = file->shared_memory->head.first_occupied;
	int current_offset = file->shared_memory->messages[current].offset;
	int search_offset = file->shared_memory->messages[search].offset;

		// Si current est la premiere case de la LC
	if(current == search){
			// Si current est aussi la derniere case de la LC
		if(file->shared_memory->messages[current].offset == 0){
			file->shared_memory->head.first_occupied = -1;
			file->shared_memory->head.last_occupied = -1;
		}
		else{
			file->shared_memory->head.first_occupied = current + current_offset;
		}
	}
	else{
		while(search != current){
			// Si current est la prochaine case de la LC
			if(search + search_offset == current){
				// Si current est la derniere case de la LC
				if(current_offset == 0) {
					file->shared_memory->messages[search].offset = 0;
					file->shared_memory->head.last_occupied = search;
				}
				else{
					file->shared_memory->messages[search].offset = search_offset + current_offset;
				}
			}
			search = search + search_offset;
			search_offset = file->shared_memory->messages[search].offset;
		}
	}
	// MAJ de la liste chainee de cases vides
	int last_free = file->shared_memory->head.last_free;

	if(last_free == -1) { file->shared_memory->head.first_free = current; }
	else{ file->shared_memory->messages[last_free].offset = last_free - current; }
	file->shared_memory->head.last_free = current;
	file->shared_memory->messages[current].offset = 0;


	// Copie et "suppression" du message
	memcpy(msg, file->shared_memory->messages[current].mtext, msg_size);

	// Synchronise la memoire
	if(msync(file, sizeof(MESSAGE), MS_SYNC) == -1) {perror("Function msync()"); exit(-1);}

	// Unlock le mutex
	if(pthread_mutex_unlock(&file->shared_memory->head.mutex) != 0){ perror("UNlock mutex"); exit(-1); }

	// Signale des processus attendant de pouvoir envoyer ou recevoir
	if(pthread_cond_signal(&file->shared_memory->head.wcond) > 0){perror("signal wcond"); exit(-1);}
	if(pthread_cond_signal(&file->shared_memory->head.rcond) > 0){perror("signal rcond"); exit(-1);}

	return msg_size;
}
