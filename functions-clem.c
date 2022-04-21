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

int enough_space(MESSAGE *file, size_t len){
	int count = 0;
	int current = file->shared_memory->head.first_free;

	if(current == -1){ return -1; }

	while(true){
		// Retourne current si la case a la place pour le message
		if(((mon_message *)file->shared_memory->messages)[current].length >= sizeof(mon_message) + len){
			return current;
		}
		// Sinon passe a la case libre suivante s'il y en a une
		else if(current != file->shared_memory->head.last_free){
			current = current + ((mon_message *)file->shared_memory->messages)[current].offset;
		}
		// Sinon aligne tous les messages a gauche (si ca n'a pas deja ete fait)
		else if(count == 0){
			//debug : fct_met_tout_a_gauche
			count = count + 1;
		}
		else { return -1; }
	}
}

int m_envoi(MESSAGE *file, const void *msg, size_t len, int msgflag){
	// Traitement des erreurs
	if(m_envoi_erreurs(file, msg, len, msgflag) < 0){ exit(-1); }

	// Lock du mutex
	if(pthread_mutex_lock(&file->shared_memory->head.mutex) != 0){ perror("lock mutex"); exit(-1); }

	// Attente si tableau plein (sauf si O_NONBLOCK)
	int current;

	while((current = enough_space(file, len)) == -1){
		if(msgflag == O_NONBLOCK) {
			my_error("Le tableau est plein (envoi en mode non bloquant).\n", file, NO_UNLOCK, 'b', EAGAIN);
		}
		if(pthread_cond_wait(&file->shared_memory->head.wcond, &file->shared_memory->head.mutex) > 0) {
			perror("wait wcond"); exit(-1);
		}
	}

	int free_space = ((mon_message *)file->shared_memory->messages)[current].length - sizeof(mon_message) - len;
	int first_free = file->shared_memory->head.first_free;
	int offset_free = ((mon_message *)file->shared_memory->messages)[current].offset;

	// Si la case libre choisie est la premiere de la LC
	if(current == first_free){
		// S'il y a assez de place pour stocker ce message plus un autre
		if(free_space > sizeof(mon_message)){
			file->shared_memory->head.first_free += sizeof(mon_message) + len;
			((mon_message *)file->shared_memory->messages)[first_free].length -= sizeof(mon_message) + len;

			if(offset_free != 0){
				((mon_message *)file->shared_memory->messages)[first_free].offset -= sizeof(mon_message) + len ;
			}
		}
		// Sinon s'il y a une autre case libre dans la LC
		else if(offset_free != 0){
			file->shared_memory->head.first_free = first_free + offset_free;
		}
		// Sinon (s'il n'y a plus de cases libres)
		else{
			file->shared_memory->head.first_free = -1;
			file->shared_memory->head.last_free = -1;
		}
	}
	// Si la case libre choisie n'est pas la premiere de la LC
	else{
		int prev = file->shared_memory->head.first_free;

		while(prev + ((mon_message *)file->shared_memory->messages)[prev].offset != cell){
			prev = prev + ((mon_message *)file->shared_memory->messages)[prev].offset;
		}
	}



	// Met a jour 'first_free', 'first_occupied' et 'last_occupied'
	int current = file->shared_memory->head.first_free;
	int offset_free = ((mon_message *)file->shared_memory->messages)[current].offset;
	((mon_message *)file->shared_memory->messages)[current].offset = 0;

	// Si le tableau est plein une fois l'envoi fait
	if(offset_free == 0){ file->shared_memory->head.first_free = -1; }
	else{ file->shared_memory->head.first_free = current + offset_free; }

	// Si le tableau etait vide avant l'envoi
	if(file->shared_memory->head.first_occupied == -1) { file->shared_memory->head.first_occupied = current; }
	else{
		int last_occupied = file->shared_memory->head.last_occupied;
		((mon_message *)file->shared_memory->messages)[last_occupied].offset = last_occupied - current;

	}
	file->shared_memory->head.last_occupied = current;

	// Unlock le mutex
	if(pthread_mutex_unlock(&file->shared_memory->head.mutex) != 0){ perror("UNlock mutex"); exit(-1); }

	// Signale un processus attendant de pouvoir envoyer
	if(pthread_cond_signal(&file->shared_memory->head.wcond) > 0){perror("signal wcond"); exit(-1);}

	// Ecrit le message dans la memoire partagee
	printf("type envoye : %ld \n", ((mon_message *)msg)->type);//debug
	memcpy(&((mon_message *)file->shared_memory->messages)[current], (mon_message *)msg, sizeof(mon_message) + len);
	((mon_message *)file->shared_memory->messages)[current].type = ((mon_message *)msg)->length = len;

	// DEBUG
	printf("La valeur du type est %ld.\n", ((mon_message *)file->shared_memory->messages)[current].type);
	printf("Le msg est %s.\n", ((mon_message *)file->shared_memory->messages)[current].mtext);
	printf("La longueur du msg est %ld.\n", ((mon_message *)file->shared_memory->messages)[current].length);
	// FIN DEBUG

	// Synchronise la memoire
	if(msync(file->shared_memory, sizeof(file->memory_size), MS_SYNC) == -1) {perror("m_envoi() -> msync()"); exit(-1);}

	// Signale un processus attendant de pouvoir recevoir
	if(pthread_cond_signal(&file->shared_memory->head.rcond) > 0){perror("signal rcond"); exit(-1);}

	return 0;
}



// Quand on supprime un message et qu'on ajoute une case a la LC des cases vides, on peut verifier si
// offset = length pour une case ou offset = - length pour l'autre case. Ca veut alors dire qu'elles sont
// concomitente et qu'on peut donc faire une seule grande case

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
				if((type>0 && ((mon_message *)file->shared_memory->messages)[current].type == type)
						|| (type<0 && ((mon_message *)file->shared_memory->messages)[current].type <= -type)){
					msg_to_read = true;
					break;
				}
				else{
					if(current == file->shared_memory->head.last_occupied) { break; }
					current = current + ((mon_message *)file->shared_memory->messages)[current].offset;
				}
			}
		}
		// Exit si pas de message et mode non bloquant
		if(flags == O_NONBLOCK && !msg_to_read){
			return my_error("Pas de message \'type\' (mode non bloquant).\n", file, UNLOCK, 'b', EAGAIN);
		}
		// Attente si pas de message et mode bloquant
		else if(!msg_to_read){
			if(pthread_cond_wait(&file->shared_memory->head.rcond, &file->shared_memory->head.mutex) > 0) {
				perror("wait wcond"); exit(-1);
			}
		}
	}
	msg_size = ((mon_message *)file->shared_memory->messages)[current].length;
	if(msg_size > len){
		return my_error("Memoire allouee trop petite pour recevoir le message.", file, UNLOCK, 'b', EMSGSIZE);
	}
	// MAJ de la liste chainee de cases occupee
	int search = file->shared_memory->head.first_occupied;
	int current_offset = ((mon_message *)file->shared_memory->messages)[current].offset;
	int search_offset = ((mon_message *)file->shared_memory->messages)[search].offset;

		// Si current est la premiere case de la LC
	if(current == search){
			// Si current est aussi la derniere case de la LC
		if(((mon_message *)file->shared_memory->messages)[current].offset == 0){
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
					((mon_message *)file->shared_memory->messages)[search].offset = 0;
					file->shared_memory->head.last_occupied = search;
				}
				else{
					((mon_message *)file->shared_memory->messages)[search].offset = search_offset + current_offset;
				}
			}
			search = search + search_offset;
			search_offset = ((mon_message *)file->shared_memory->messages)[search].offset;
		}
	}
	// MAJ de la liste chainee de cases vides
	int last_free = file->shared_memory->head.last_free;

	if(last_free == -1) { file->shared_memory->head.first_free = current; }
	else{ ((mon_message *)file->shared_memory->messages)[last_free].offset = last_free - current; }
	file->shared_memory->head.last_free = current;
	((mon_message *)file->shared_memory->messages)[current].offset = 0;


	// Copie et "suppression" du message
	memcpy((mon_message *)msg, &((mon_message *)file->shared_memory->messages)[current], sizeof(mon_message) + msg_size);

	// Synchronise la memoire
	if(msync(file->shared_memory, sizeof(file->memory_size), MS_SYNC) == -1) {perror("Function msync()"); exit(-1);}

	// Unlock le mutex
	if(pthread_mutex_unlock(&file->shared_memory->head.mutex) != 0){ perror("UNlock mutex"); exit(-1); }

	// Signale des processus attendant de pouvoir envoyer ou recevoir
	if(pthread_cond_signal(&file->shared_memory->head.wcond) > 0){perror("signal wcond"); exit(-1);}
	if(pthread_cond_signal(&file->shared_memory->head.rcond) > 0){perror("signal rcond"); exit(-1);}

	return msg_size;
}
