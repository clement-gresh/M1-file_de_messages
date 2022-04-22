#include "m_file.h"

// <>


int my_error(char *txt, MESSAGE *file, bool unlock, char signal, int error){
	struct header *head = &file->shared_memory->head;
	perror(txt);
	if(unlock){
		if(pthread_mutex_unlock(&head->mutex) != 0){ perror("UNlock mutex"); exit(-1); }
	}
	if(signal=='r' || signal == 'b'){
		if(pthread_cond_signal(&head->rcond) > 0){perror("signal rcond"); exit(-1);}
	}
	if(signal=='w' || signal == 'b'){
		if(pthread_cond_signal(&head->wcond) > 0){perror("signal wcond"); exit(-1);}
	}
	if(error > 0) { errno = error; }
	return(-1);
}

// Debug : my_error a utiliser
int m_envoi_erreurs(MESSAGE *file, const void *msg, size_t len, int msgflag){
	struct header *head = &file->shared_memory->head;

	if(file->flag == O_RDONLY){
		printf("Impossible d'ecrire dans cette file.\n");
		errno = EPERM;
		exit(-1);
	}
	if(len > head->max_length_message){
		printf("La taille du message excede la taille maximale autorisee.\n");
		errno = EMSGSIZE;
		exit(-1);
	}
	if(msgflag != 0 && msgflag != O_NONBLOCK){
		printf("Valeur de msgflag incorrecte.\n");
		errno = EIO;
		exit(-1);
	}
	if((head->first_free == -1) && (msgflag == O_NONBLOCK)){
		printf("Le tableau est plein (envoi en mode non bloquant).\n");
		errno = EAGAIN;
		exit(-1);
	}
	return 0;
}

int enough_space(MESSAGE *file, size_t len){
	mon_message *messages = (mon_message *)file->shared_memory->messages;
	struct header *head = &file->shared_memory->head;
	int count = 0;
	int current = head->first_free;

	if(current == -1){ return -1; }

	while(true){
		// Retourne current si la case a la place pour le message
		if(messages[current].length >= sizeof(mon_message) + len){
			return current;
		}
		// Sinon passe a la case libre suivante s'il y en a une
		else if(current != head->last_free){
			current = current + messages[current].offset;
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
	struct mon_message *messages = (mon_message *)file->shared_memory->messages;
	struct header *head = &file->shared_memory->head;

	// Traitement des erreurs
	if(m_envoi_erreurs(file, msg, len, msgflag) < 0){ exit(-1); }

	// Lock du mutex
	if(pthread_mutex_lock(&head->mutex) != 0){ perror("lock mutex"); exit(-1); }

	// Attente si tableau plein (sauf si O_NONBLOCK)
	int current;


	while((current = enough_space(file, len)) == -1){
		if(msgflag == O_NONBLOCK) {
			return my_error("Le tableau est plein (envoi en mode non bloquant).\n", file, NO_UNLOCK, 'b', EAGAIN);
		}
		if(pthread_cond_wait(&head->wcond, &head->mutex) > 0) {
			perror("wait wcond"); exit(-1);
		}
	}

	// MAJ DE LA LISTE CHAINEE DES CASES LIBRES
	int first_free = head->first_free;
	int last_free = head->last_free;
	int prev = first_free;

	if(prev != current){
		while(prev + messages[prev].offset != current){
			prev = prev + messages[prev].offset;
		}
	}

	int free_space = messages[current].length - sizeof(mon_message) - len;
	int current_offset = messages[current].offset;
	int current_length = messages[current].length;
	size_t msg_size = sizeof(mon_message) + len;


	// Si current a assez de place libre pour stocker ce message plus un autre
	if(free_space > sizeof(mon_message)){
		// "Creation" de next
		int next = current + msg_size;
		messages[next].length = current_length - msg_size;

		if(current_offset != 0){ messages[next].offset = current_offset - msg_size; }
		else{ messages[next].offset = 0; }

		// MAJ de prev
		if(head->first_free == current){ head->first_free = next; }
		else{ messages[prev].offset += msg_size; }
	}

	// Sinon si current est la premiere case de la LC
	else if(prev == current){
		// S'il y a une autre case libre dans la LC ou non
		if(current_offset != 0){ head->first_free = current + current_offset; }
		else{ head->first_free = -1; }
	}

	// Sinon (si current n'est pas la premiere case de la LC)
	else{
		// S'il y a une autre case libre apres current dans la LC
		if(current_offset != 0){
			messages[prev].offset += current_offset;
		}
		// Si current etait la derniere case libre de la LC
		else{
			messages[prev].offset = 0;
			head->last_free = prev;
		}

		if(current_offset == 0){ messages[prev].offset = 0; }
		else{ messages[prev].offset += current_offset; }
	}

	if(first_free == last_free){ head->last_free = head->first_free; }


	// MAJ DE LA LISTE CHAINEE DES CASES OCCUPEES
	int first_occupied = head->first_occupied;
	int last_occupied = head->last_occupied;

	if(first_occupied == -1){ head->first_occupied = current; }
	else{ messages[last_occupied].offset = last_occupied - current; }

	head->last_occupied = current;

	// Unlock le mutex
	if(pthread_mutex_unlock(&head->mutex) != 0){ perror("UNlock mutex"); exit(-1); }

	// Signale un processus attendant de pouvoir envoyer
	if(pthread_cond_signal(&head->wcond) > 0){perror("signal wcond"); exit(-1);}

	// Ecrit le message dans la memoire partagee
	printf("Type message envoye : %ld \n", ((mon_message *)msg)->type);//debug
	memcpy(&messages[current], (mon_message *)msg, msg_size);
	messages[current].length = len;
	messages[current].offset = 0; // Le message occupe forcement la derniere case de la LC de cases occupees

	// DEBUG
	printf("Type : %ld\n", messages[current].type);
	printf("Length : %ld\n", messages[current].length);
	printf("Offset : %ld\n", messages[current].offset);
	printf("Message : %d\n", ((int*)messages[current].mtext)[0]);
	printf("\n");
	// FIN DEBUG

	// Synchronise la memoire
	if(msync(file->shared_memory, sizeof(file->memory_size), MS_SYNC) == -1) {
		perror("m_envoi() -> msync()"); exit(-1);
	}

	// Signale un processus attendant de pouvoir recevoir
	if(pthread_cond_signal(&head->rcond) > 0){perror("signal rcond"); exit(-1);}

	return 0;
}

ssize_t m_reception(MESSAGE *file, void *msg, size_t len, long type, int flags){
	mon_message *messages = (mon_message *)file->shared_memory->messages;
	struct header *head = &file->shared_memory->head;

	// Erreurs
	if(file->flag == O_WRONLY){
		printf("Impossible de lire les message de cette file.\n");
		errno = EPERM;
		exit(-1);
	}

	// Lock du mutex
	if(pthread_mutex_lock(&head->mutex) != 0){ perror("lock mutex"); exit(-1); }

	// Recherche d'un message a lire
	int current;
	bool msg_to_read = false;

	while(!msg_to_read){
		current = head->first_occupied;

		// Verifie s'il y a un message correspondant a 'type' dans la file
		if(type==0 && current != -1){
			msg_to_read = true;
		}
		else if(type != 0 && current != -1){
			while(true){
				if((type>0 && messages[current].type == type)
						|| (type<0 && messages[current].type <= -type)){
					msg_to_read = true;
					break;
				}
				else{
					if(current == head->last_occupied) { break; }
					current = current + messages[current].offset;
				}
			}
		}
		// Exit si pas de message et mode non bloquant
		if(flags == O_NONBLOCK && !msg_to_read){
			return my_error("Pas de message \'type\' (mode non bloquant).\n", file, UNLOCK, 'b', EAGAIN);
		}
		// Attente si pas de message et mode bloquant
		else if(!msg_to_read){
			if(pthread_cond_wait(&head->rcond, &head->mutex) > 0) {
				perror("wait wcond"); exit(-1);
			}
		}
	}

	// Erreur si buffer trop petit pour recevoir le message
	ssize_t msg_size = messages[current].length;
	if(msg_size > len){
		return my_error("Memoire allouee trop petite pour recevoir le message.", file, UNLOCK, 'b', EMSGSIZE);
	}


	// MAJ de la liste chainee de cases occupee



	// MAJ de la liste chainee des cases vides
	int first_free = head->first_free;
	int last_free = head->last_free;

	if(first_free == -1){ head->first_free = current; }
	else{ messages[last_free].offset = last_free - current; }

	head->last_free = current;
	messages[current].offset = 0;  // Le case est forcement la derniere case de la LC des cases libres

	/*
	// MAJ de la liste chainee de cases occupee
	int search = head->first_occupied;
	int current_offset = messages[current].offset;
	int search_offset = messages[search].offset;

		// Si current est la premiere case de la LC
	if(current == search){
			// Si current est aussi la derniere case de la LC
		if(messages[current].offset == 0){
			head->first_occupied = -1;
			head->last_occupied = -1;
		}
		else{
			head->first_occupied = current + current_offset;
		}
	}
	else{
		while(search != current){
			// Si current est la prochaine case de la LC
			if(search + search_offset == current){
				// Si current est la derniere case de la LC
				if(current_offset == 0) {
					messages[search].offset = 0;
					head->last_occupied = search;
				}
				else{
					messages[search].offset = search_offset + current_offset;
				}
			}
			search = search + search_offset;
			search_offset = messages[search].offset;
		}
	}
	// MAJ de la liste chainee de cases vides
	int last_free = head->last_free;

	if(last_free == -1) { head->first_free = current; }
	else{ messages[last_free].offset = last_free - current; }
	head->last_free = current;
	messages[current].offset = 0;
	*/

	// Copie et "suppression" du message
	memcpy((mon_message *)msg, &messages[current], sizeof(mon_message) + msg_size);

	// Synchronise la memoire
	if(msync(file->shared_memory, sizeof(file->memory_size), MS_SYNC) == -1) {perror("Function msync()"); exit(-1);}

	// Unlock le mutex
	if(pthread_mutex_unlock(&head->mutex) != 0){ perror("UNlock mutex"); exit(-1); }

	// Signale des processus attendant de pouvoir envoyer ou recevoir
	if(pthread_cond_signal(&head->wcond) > 0){perror("signal wcond"); exit(-1);}
	if(pthread_cond_signal(&head->rcond) > 0){perror("signal rcond"); exit(-1);}

	return msg_size;
}
