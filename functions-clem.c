#include "m_file.h"

// <>

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
	size_t msg_size = sizeof(mon_message) + len;
	// Verification de l'absence d'erreurs dans les paramtres d'appel
	if(m_envoi_erreurs(file, len, msgflag) != 0){ return -1; }

	// Lock du mutex
	if(pthread_mutex_lock(&head->mutex) != 0){ perror("lock mutex"); exit(-1); }

	// Attente si tableau plein (sauf si O_NONBLOCK)
	int current = m_envoi_recherche(file, len, msgflag);
	if(current == -1) { return -1; }

	// Maj de la liste chainee des cases libres
	m_envoi_libres(file, current, len);

	// Unlock le mutex
	if(pthread_mutex_unlock(&head->mutex) != 0){ perror("UNlock mutex"); exit(-1); }

	// Signale un processus attendant de pouvoir envoyer
	if(pthread_cond_signal(&head->wcond) > 0){perror("signal wcond"); exit(-1);}

	// Ecrit le message dans la memoire partagee
	memcpy(&messages[current], (mon_message *)msg, msg_size);
	messages[current].length = len;
	messages[current].offset = 0; // Le message occupe forcement la derniere case de la LC de cases occupees

	// Maj de la liste chainee des cases occupees
	m_envoi_occupees(file, current);

	// Synchronise la memoire
	if(msync(file->shared_memory, sizeof(file->memory_size), MS_SYNC) == -1) { perror("m_envoi() -> msync()"); exit(-1); }

	// Signale un processus attendant de pouvoir recevoir
	if(pthread_cond_signal(&head->rcond) > 0) {perror("signal rcond"); exit(-1); }

	return 0;
}



ssize_t m_reception(MESSAGE *file, void *msg, size_t len, long type, int flags){
	mon_message *messages = (mon_message *)file->shared_memory->messages;
	header *head = &file->shared_memory->head;

	// Verification de l'absence d'erreurs dans les paramtres d'appel
	if(m_reception_erreurs(file, flags) != 0){ return -1; }

	// Lock du mutex
	if(pthread_mutex_lock(&head->mutex) != 0){ perror("lock mutex"); exit(-1); }

	// Recherche d'un message a lire
	int current = m_reception_recherche(file, type, flags);
	if(current == -1) { return -1; }

	// Erreur si buffer trop petit pour recevoir le message
	ssize_t msg_size = messages[current].length;
	if(msg_size > len){
		return my_error("Memoire allouee trop petite pour recevoir le message.", file, UNLOCK, 'b', EMSGSIZE);
	}

	// Maj de la liste chainee des cases occupees
	 m_reception_occupees(file, current);

	// Unlock le mutex
	if(pthread_mutex_unlock(&head->mutex) != 0){ perror("UNlock mutex"); exit(-1); }

	// Signale des processus attendant de pouvoir recevoir
	if(pthread_cond_signal(&head->rcond) > 0){perror("signal rcond"); exit(-1);}

	// Copie du message
	memcpy((mon_message *)msg, &messages[current], sizeof(mon_message) + msg_size);

	// Maj de la liste chainee des cases libres
	m_reception_libres(file, current);

	// Synchronise la memoire
	if(msync(file->shared_memory, sizeof(file->memory_size), MS_SYNC) == -1){ perror("Function msync()"); exit(-1); }

	// Signale des processus attendant de pouvoir envoyer
	if(pthread_cond_signal(&head->rcond) > 0){ perror("signal wcond"); exit(-1); }

	return msg_size;
}


// Renvoie un message d'erreur et, si besoin, unlock le mutex, signale une/les condition(s) et assigne une valeur a errno
int my_error(char *txt, MESSAGE *file, bool unlock, char signal, int error){
	struct header *head = &file->shared_memory->head;
	printf("%s", txt);

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
	return -1;
}


// FONCTIONS AUXILIAIRES DE M_ENVOI

// Verifie l'absence d'erreur dans les parametres d'appel de m_envoi
int m_envoi_erreurs(MESSAGE *file, size_t len, int msgflag){
	struct header *head = &file->shared_memory->head;

	if(file->flag == O_RDONLY){
		return my_error("Impossible d'ecrire dans cette file.\n", file, NO_UNLOCK, 'w', EPERM);
	}
	if(len > head->max_length_message){
		return my_error("La taille du message excede la taille maximale.\n", file, NO_UNLOCK, 'b', EMSGSIZE);
	}
	if(msgflag != 0 && msgflag != O_NONBLOCK){
		return my_error("Valeur de msgflag incorrecte dans m_envoi.\n", file, NO_UNLOCK, 'b', EIO);
	}
	return 0;
}


// Maj de la liste chainee des cases libres dans m_envoi
void m_envoi_libres(MESSAGE *file, int current, size_t len){
	struct mon_message *messages = (mon_message *)file->shared_memory->messages;
	struct header *head = &file->shared_memory->head;

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
}


// Maj de la liste chainee des cases occupees dans m_envoi
void m_envoi_occupees(MESSAGE *file, int current){
	struct mon_message *messages = (mon_message *)file->shared_memory->messages;
	struct header *head = &file->shared_memory->head;

	int first_occupied = head->first_occupied;
	int last_occupied = head->last_occupied;

	if(first_occupied == -1){ head->first_occupied = current; }
	else{ messages[last_occupied].offset = current - last_occupied;	}

	head->last_occupied = current;
}


// Recherche d'une case dans laquelle ecrire et attente si besoin (ou exit en mode non-bloquant)
int m_envoi_recherche(MESSAGE *file, size_t len, int msgflag){
	struct header *head = &file->shared_memory->head;
	int current;

	while((current = enough_space(file, len)) == -1){
		if(msgflag == O_NONBLOCK) {
			return my_error("Le tableau est plein (envoi en mode non bloquant).\n", file, UNLOCK, 'b', EAGAIN);
		}
		if(pthread_cond_wait(&head->wcond, &head->mutex) > 0) { perror("wait wcond"); exit(-1); }
	}
	return current;
}


// FONCTIONS AUXILIAIRES DE M_RECHERCHE

// Verifie l'absence d'erreur dans les parametres d'appel de m_reception
int m_reception_erreurs(MESSAGE *file, int flags){
	if(file->flag == O_WRONLY){
		return my_error("Impossible de lire les message de cette file.\n", file, NO_UNLOCK, 'w', EPERM);
	}
	if(flags != 0 && flags != O_NONBLOCK){
		return my_error("Valeur de msgflag incorrecte dans m_reception.\n", file, NO_UNLOCK, 'b', EIO);
	}
	return 0;
}


// Maj de la liste chainee des cases occupees dans m_reception
void m_reception_occupees(MESSAGE *file, int current){
	struct mon_message *messages = (mon_message *)file->shared_memory->messages;
	struct header *head = &file->shared_memory->head;

	int prev = head->first_occupied;
	int current_offset = messages[current].offset;
	int prev_offset = messages[prev].offset;

	// Si current est la premiere case de la LC
	if(current == prev){
		// Si current est aussi la derniere case de la LC
		if(messages[current].offset == 0){
			head->first_occupied = -1;
			head->last_occupied = -1;
		}
		else{
			head->first_occupied = current + current_offset;
		}
	}
	// Sinon si current n'est pas la premiere case de la LC
	else{
		while(prev != current){
			// Si current est la prochaine case de la LC
			if(prev + prev_offset == current){
				// Si current est la derniere case de la LC
				if(current_offset == 0) {
					messages[prev].offset = 0;
					head->last_occupied = prev;
				}
				else{
					messages[prev].offset = prev_offset + current_offset;
				}
			}
			prev = prev + prev_offset;
			prev_offset = messages[prev].offset;
		}
	}
}


// Maj de la liste chainee des cases occlibresupees dans m_reception
void m_reception_libres(MESSAGE *file, int current){
	struct mon_message *messages = (mon_message *)file->shared_memory->messages;
	struct header *head = &file->shared_memory->head;

	int last_free = head->last_free;

	if(last_free == -1){ head->first_free = current; }
	else{ messages[last_free].offset = current- last_free; }

	head->last_free = current;
	messages[current].offset = 0;  // Le case est forcement la derniere case de la LC des cases libres
}


// Recherche d'un message a lire et attente si besoin (ou exit en mode non-bloquant)
int m_reception_recherche(MESSAGE *file, long type, int flags){
	struct mon_message *messages = (mon_message *)file->shared_memory->messages;
	struct header *head = &file->shared_memory->head;

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
				if((type>0 && messages[current].type == type) || (type<0 && messages[current].type <= -type)){
					msg_to_read = true;
					break;
				}
				else if(current == head->last_occupied){
					 break;
				}
				else{
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
	return current;
}


