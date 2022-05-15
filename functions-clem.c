#include "m_file.h"

// <>

// debug : maj des LC des cases libres et occupees sont symetriques. Peut probablement les factoriser dans une seule fonction

int enough_space(MESSAGE *file, size_t len){
	char *messages = file->shared_memory->messages;
	struct header *head = &file->shared_memory->head;
	int count = 0;
	int current = head->first_free;

	if(current == -1){ return -1; }

	while(true){
		// Retourne current si la case a la place pour le message
		if(((mon_message *)&messages[current])->length >= len){
			return current;
		}
		// Sinon passe a la case libre suivante s'il y en a une
		else if(current != head->last_free){
			current = current + ((mon_message *)&messages[current])->offset;
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
	char *messages = file->shared_memory->messages;
	struct header *head = &file->shared_memory->head;
	size_t msg_size = sizeof(mon_message) + len;

	// Verification de l'absence d'erreurs dans les paramtres d'appel
	if(m_envoi_erreurs(file, len, msgflag) != 0){ return -1; }

	// Lock du mutex
	if(pthread_mutex_lock(&head->mutex) != 0){ perror("lock mutex"); exit(-1); }

	// Recherche d'une case libre pour ecrire et attente si tableau plein (sauf si O_NONBLOCK)
	int current = m_envoi_recherche(file, len, msgflag);
	if(current == -1) { return -1; }

	// Maj de la liste chainee des cases libres
	m_envoi_libres(file, current, len);

	// Unlock le mutex
	//if(pthread_mutex_unlock(&head->mutex) != 0){ perror("UNlock mutex"); exit(-1); } // debug

	// Signale un processus attendant de pouvoir envoyer
	if(pthread_cond_signal(&head->wcond) > 0){perror("signal wcond"); exit(-1);}

	// Ecrit le message dans la memoire partagee
	memcpy(&messages[current], (mon_message *)msg, msg_size);
	((mon_message *)&messages[current])->length = len;
	((mon_message *)&messages[current])->offset = 0; // Le message occupe forcement la derniere case de la LC de cases occupees

	// Maj de la liste chainee des cases occupees
	m_envoi_occupees(file, current);

	// Synchronise la memoire
	//if(msync(file->shared_memory, sizeof(file->memory_size), MS_SYNC) == -1) { perror("m_envoi() -> msync()"); exit(-1); }

	// Signale si besoin les processus qui sont sur la liste d'attente des notifications
	m_signal(file);

	// Signale un processus attendant de pouvoir recevoir
	if(pthread_cond_signal(&head->rcond) > 0) {perror("signal rcond"); exit(-1); }

	// DEBUG
	if(pthread_mutex_unlock(&head->mutex) != 0){ perror("UNlock mutex"); exit(-1); } // debug
	//FIN DEBUG

	return 0;
}



ssize_t m_reception(MESSAGE *file, void *msg, size_t len, long type, int flags){
	char *messages = file->shared_memory->messages;
	header *head = &file->shared_memory->head;

	// Verification de l'absence d'erreurs dans les paramtres d'appel
	if(m_reception_erreurs(file, flags) != 0){ return -1; }

	// Lock du mutex
	if(pthread_mutex_lock(&head->mutex) != 0){ perror("lock mutex"); exit(-1); }

	// Recherche d'un message a lire et attente s'il n'y en a pas (sauf si O_NONBLOCK)
	int current = m_reception_recherche(file, type, flags);
	if(current == -1) { return -1; }

	// Erreur si buffer trop petit pour recevoir le message
	ssize_t msg_size = ((mon_message *)&messages[current])->length;
	if(msg_size > len){
		return my_error("Memoire allouee trop petite pour recevoir le message.\n", file, type, UNLOCK, 'b', EMSGSIZE);
	}

	// Maj de la liste chainee des cases occupees
	m_reception_occupees(file, current);

	// Unlock le mutex
	//if(pthread_mutex_unlock(&head->mutex) != 0){ perror("UNlock mutex"); exit(-1); }

	// Signale des processus attendant de pouvoir recevoir
	if(pthread_cond_signal(&head->rcond) > 0){perror("signal rcond"); exit(-1); }

	// Copie du message
	memcpy((mon_message *)msg, &messages[current], sizeof(mon_message) + msg_size);

	// Maj de la liste chainee des cases libres
	m_reception_libres(file, current);

	// Synchronise la memoire
	//if(msync(file->shared_memory, sizeof(file->memory_size), MS_SYNC) == -1){ perror("Function msync()"); exit(-1); }

	// Signale si besoin les processus qui sont sur la liste d'attente des notifications
	m_signal(file);

	// Signale des processus attendant de pouvoir envoyer
	if(pthread_cond_signal(&head->rcond) > 0){ perror("signal wcond"); exit(-1); }

	// DEBUG
	if(pthread_mutex_unlock(&head->mutex) != 0){ perror("UNlock mutex"); exit(-1); }
	//FIN DEBUG

	return msg_size;
}


// Renvoie la taille maximale d'un message
size_t m_message_len(MESSAGE *file){
	return file->shared_memory->head.max_length_message;
}

// Renvoie le nombre minimum de messages que peut contenir la file
size_t m_capacite(MESSAGE *file){
	return file->shared_memory->head.pipe_capacity;
}

// Renvoie le nombre de messages actuellement dans la file
size_t m_nb(MESSAGE *file ){
	int current = file->shared_memory->head.first_occupied;
	if(current == -1) { return 0; }

	int msg_nb = 1;
	int offset = ((mon_message*)&file->shared_memory->messages[current])->offset;

	while(offset != 0){
		current = current + offset;
		offset = ((mon_message*)&file->shared_memory->messages[current])->offset;
		msg_nb = msg_nb + 1;
	}
	return msg_nb;
}

// Enregistre sur la file d'attente un processus qui souhaite recevoir un signal 'sig' lorsqu'un message
// de type 'type' est disponible
// Retour : 0 en cas de succès, -1 sinon
int m_enregistrement(MESSAGE *file, long type, int sig){
	struct header *head = &file->shared_memory->head;
	int current = 0;

	while(current < RECORD_NB && head->records[current].pid != -1){
		current = current + 1;
	}
	if(current != RECORD_NB){
		head->records[current].pid = getpid();
		head->records[current].signal = sig;
		head->records[current].type = type;
		return 0;
	}
	return -1;
}

// Enleve le processus de la file d'attente (y compris s'il y apparait plusieurs fois)
void m_annulation(MESSAGE *file){
	struct header *head = &file->shared_memory->head;

	for(int i = 0; i < RECORD_NB; i++){
		if(head->records[i].pid == getpid()) {
			head->records[i].pid = -1;
		}
	}
}


// FONCTIONS AUXILIAIRES
// Renvoie un message d'erreur et, si besoin, unlock le mutex, signale une/les condition(s) et assigne une valeur a errno
int my_error(char *txt, MESSAGE *file, long type, bool unlock, char signal, int error){
	struct header *head = &file->shared_memory->head;
	printf("%s", txt);

	if(unlock){
		if(pthread_mutex_unlock(&head->mutex) != 0){ perror("UNlock mutex"); exit(-1); }
	}
	if(signal=='r' || signal == 'b'){
		if(pthread_cond_signal(&head->rcond) > 0){ perror("signal rcond"); exit(-1); }
	}
	if(signal=='w' || signal == 'b'){
		if(pthread_cond_signal(&head->wcond) > 0){ perror("signal wcond"); exit(-1); }
	}
	if(error > 0) { errno = error; }
	return -1;
}

// FONCTIONS AUXILIAIRES DE M_ENREGISTREMENT
// Envoi un signal aux processus enregistres sur la file d'attente apres avoir verifie si les conditions sont satisfaites
void m_signal(MESSAGE *file){
	struct header *head = &file->shared_memory->head;

	for(int i = 0; i < RECORD_NB; i++){
		// Recherche si un processus est enregistre pour ce type de message
		if(head->records[i].pid != -1){
			bool proc_waiting = false;
			int k = 0;

			// Verifie qu'il n'y a aucun pocessus suspendu en attente de ce message
			while(k < TYPE_SEARCH_NB){
				if(head->types_searched[k].number > 0 &&
						(head->types_searched[k].type == 0
						|| head->types_searched[k].type == head->records[i].type
						|| -head->types_searched[k].type > head->records[i].type))
				{
					proc_waiting = true;
					break;
				}
				k++;
			}
			if(!proc_waiting){
				// Verifie s'il y a un message correspondant a 'type' dans la file
				if(is_type_available(file, head->records[i].type) != -1){

					// Envoie le signal au processus si les conditions sont verifies
					kill(head->records[i].pid, head->records[i].signal);
					head->records[i].pid = -1;
				}
			}
		}
	}
}

// Determine si un message correspondant au type voulu est disponible dans la liste chainee des cases occupees
// Retourne la position du message si c'est le cas, -1 sinon
int is_type_available(MESSAGE *file, long type){
	char *messages = file->shared_memory->messages;
	struct header *head = &file->shared_memory->head;

	int current = head->first_occupied;

	// Verifie s'il y a un message correspondant a 'type' dans la file
	if(type==0 && current != -1){
		return current;
	}
	else if(type != 0 && current != -1){
		while(true){
			if((type>0 && ((mon_message *)&messages[current])->type == type)
					|| (type<0 && ((mon_message *)&messages[current])->type <= -type)){
				return current;
			}
			else if(current == head->last_occupied){
				return -1;
			}
			else{
				current = current + ((mon_message *)&messages[current])->offset;
			}
		}
	}
	return -1;
}



// FONCTIONS AUXILIAIRES DE M_ENVOI

// Verifie l'absence d'erreur dans les parametres d'appel de m_envoi
int m_envoi_erreurs(MESSAGE *file, size_t len, int msgflag){
	struct header *head = &file->shared_memory->head;

	if(file->flag == O_RDONLY){
		return my_error("Impossible d'ecrire dans cette file.\n", file, 0, NO_UNLOCK, 'n', EPERM);
	}
	if(len > head->max_length_message){
		return my_error("La taille du message excede la taille maximale.\n", file, 0, NO_UNLOCK, 'b', EMSGSIZE);
	}
	if(msgflag != 0 && msgflag != O_NONBLOCK){
		return my_error("Valeur de msgflag incorrecte dans m_envoi.\n", file, 0, NO_UNLOCK, 'b', EIO);
	}
	return 0;
}


// Maj de la liste chainee des cases libres dans m_envoi
void m_envoi_libres(MESSAGE *file, int current, size_t len){
	char *messages = file->shared_memory->messages;
	struct header *head = &file->shared_memory->head;

	int first_free = head->first_free;
	int last_free = head->last_free;
	int prev = first_free;

	if(prev != current){
		while(prev + ((mon_message *)&messages[prev])->offset != current){
			prev = prev + ((mon_message *)&messages[prev])->offset;
		}
	}

	int free_space = ((mon_message *)&messages[current])->length - sizeof(mon_message) - len;
	int current_offset = ((mon_message *)&messages[current])->offset;
	int current_length = ((mon_message *)&messages[current])->length;
	size_t msg_size = sizeof(mon_message) + len;

	// Si current a assez de place libre pour stocker ce message plus un autre
	if(free_space > 0){
		// "Creation" de next
		int next = current + msg_size;
		((mon_message *)&messages[next])->length = current_length - msg_size;

		if(current_offset != 0){ ((mon_message *)&messages[next])->offset = current_offset - msg_size; }
		else{ ((mon_message *)&messages[next])->offset = 0; }

		// MAJ de prev
		if(head->first_free == current){ head->first_free = next; }
		else{ ((mon_message *)&messages[prev])->offset += msg_size; }
	}

	// Sinon si current est la premiere case de la LC
	else if(prev == current){
		// Selon qu'il y a une autre case libre dans la LC ou non
		if(current_offset != 0){ head->first_free = current + current_offset; }
		else{ head->first_free = -1; }
	}

	// Sinon (si current n'est pas la premiere case de la LC)
	else{
		// S'il y a une autre case libre apres current dans la LC
		if(current_offset != 0){
			((mon_message *)&messages[prev])->offset += current_offset;
		}
		// Si current etait la derniere case libre de la LC
		else{
			((mon_message *)&messages[prev])->offset = 0;
			head->last_free = prev;
		}
		if(current_offset == 0){ ((mon_message *)&messages[prev])->offset = 0; }
		else{ ((mon_message *)&messages[prev])->offset += current_offset; }
	}
	if(first_free == last_free){ head->last_free = head->first_free; }
}


// Maj de la liste chainee des cases occupees dans m_envoi
void m_envoi_occupees(MESSAGE *file, int current){
	char *messages = file->shared_memory->messages;
	struct header *head = &file->shared_memory->head;

	int first_occupied = head->first_occupied;
	int last_occupied = head->last_occupied;

	if(first_occupied == -1){ head->first_occupied = current; }
	else{ ((mon_message *)&messages[last_occupied])->offset = current - last_occupied;	}

	head->last_occupied = current;
}


// Recherche d'une case dans laquelle ecrire et attente si besoin (ou exit en mode non-bloquant)
int m_envoi_recherche(MESSAGE *file, size_t len, int msgflag){
	struct header *head = &file->shared_memory->head;
	int current;

	while((current = enough_space(file, len)) == -1){
		if(msgflag == O_NONBLOCK) {
			return my_error("Le tableau est plein (envoi en mode non bloquant).\n", file, 0, UNLOCK, 'b', EAGAIN);
		}
		// Signal la condition read avant de wait sur condition la write
		if(pthread_cond_signal(&head->rcond) > 0){perror("signal rcond"); exit(-1);}
		if(pthread_cond_wait(&head->wcond, &head->mutex) > 0) { perror("wait wcond"); exit(-1); }
	}
	return current;
}



// Verifie l'absence d'erreur dans les parametres d'appel de m_reception
int m_reception_erreurs(MESSAGE *file, int flags){
	if(file->flag == O_WRONLY){
		return my_error("Impossible de lire les message de cette file.\n", file, 0, NO_UNLOCK, 'w', EPERM);
	}
	if(flags != 0 && flags != O_NONBLOCK){
		return my_error("Valeur de msgflag incorrecte dans m_reception.\n", file, 0, NO_UNLOCK, 'b', EIO);
	}
	return 0;
}


// Maj de la liste chainee des cases occupees dans m_reception
void m_reception_occupees(MESSAGE *file, int current){
	char *messages = file->shared_memory->messages;
	struct header *head = &file->shared_memory->head;

	int prev = head->first_occupied;
	int current_offset = ((mon_message *)&messages[current])->offset;
	int prev_offset = ((mon_message *)&messages[prev])->offset;

	// Si current est la premiere case de la LC
	if(current == prev){
		// Si current est aussi la derniere case de la LC
		if(((mon_message *)&messages[current])->offset == 0){
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
					((mon_message *)&messages[prev])->offset = 0;
					head->last_occupied = prev;
				}
				else{
					((mon_message *)&messages[prev])->offset = prev_offset + current_offset;
				}
			}
			prev = prev + prev_offset;
			prev_offset = ((mon_message *)&messages[prev])->offset;
		}
	}
}


// Maj de la liste chainee des cases libres dans m_reception
void m_reception_libres(MESSAGE *file, int current){
	char *messages = file->shared_memory->messages;
	struct header *head = &file->shared_memory->head;

	int last_free = head->last_free;

	if(last_free == -1){ head->first_free = current; }
	else{ ((mon_message *)&messages[last_free])->offset = current- last_free; }

	head->last_free = current;
	((mon_message *)&messages[current])->offset = 0;  // Le case est forcement la derniere case de la LC des cases libres
}


// Recherche d'un message a lire et attente si besoin (ou exit en mode non-bloquant)
int m_reception_recherche(MESSAGE *file, long type, int flags){
	struct header *head = &file->shared_memory->head;
	int current = -1;
	bool wait = false;
	int position;

	while(current == -1){
		// Verifie s'il y a un message correspondant a 'type' dans la file
		current = is_type_available(file, type);

		// Exit si pas de message et mode non bloquant
		if(flags == O_NONBLOCK && current == -1){
			return my_error("Pas de message \'type\' (mode non bloquant).\n", file, type, UNLOCK, 'b', EAGAIN);
		}
		// Attente si pas de message et mode bloquant (et signal la condition rcond)
		else if(current == -1){

			// Si c'est la premiere fois que le processus fait wait, il indique d'abord le type de message qu'il recherche
			// dans la liste types_searched
			if(wait == false){
				if((position = m_reception_type_searched(file, type)) == -1) { return -1; }
			}
			// Signal wcond avant de wait sur rcond
			wait = true;
			if(pthread_cond_signal(&head->wcond) > 0){perror("signal wcond"); exit(-1);}
			if(pthread_cond_wait(&head->rcond, &head->mutex) > 0) { perror("wait rcond"); exit(-1); }
		}
	}
	if(wait == true){
		// Si le processus a fait wait, puis a ete reveille et a trouve un message a lire,
		// il doit decrementer le nombre correspondant au type recherche dans types_searched
		head->types_searched[position].number--;
	}
	return current;
}


// Met a jour le tableau type_searched en incrementant l'effectif du 'type' de message recherche
int m_reception_type_searched(MESSAGE *file, long type){
	struct header *head = &file->shared_memory->head;
	int position;
	bool type_found = false;

	// Cherche si le 'type' recherche apparait deja dans le tableau
	for(position = 0; position < TYPE_SEARCH_NB; position++){
		if(head->types_searched[position].type == type){
			head->types_searched[position].number++;
			type_found = true;
			break;
		}
	}
	// Sinon cherche si une case a 'number' a 0 et remplace son 'type'
	if(!type_found){
		for(position = 0; position < TYPE_SEARCH_NB; position++){
			if(head->types_searched[position].number == 0){
				head->types_searched[position].type = type;
				head->types_searched[position].number = 1;
				type_found = true;
				break;
			}
		}
	}
	// Sinon indique a l'utilisateur qu'il n'y a pas de place dans le tableau et exit
	if(!type_found){
		return my_error("Pas de place dans types_searched[].\n", file, 0, UNLOCK, 'b', -1);
	}
	return position;
}
