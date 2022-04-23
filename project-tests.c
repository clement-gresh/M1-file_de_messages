#include "m_file.h"

// <>

int test_connexion(){
	char name[] = "/kangourouqkdjnqkdjfnqlfn";
	int msg_nb = 12;
	size_t max_message_length = sizeof(char)*20;
	MESSAGE* file = m_connexion(name, O_RDWR | O_CREAT, msg_nb, max_message_length, S_IRWXU | S_IRWXG | S_IRWXO);
	struct header *head = &file->shared_memory->head;

	// Verifie dans la file les attributs nom, nombre de messages et longueur max d'un message
	if(strcmp(file->name, name) != 0 || head->pipe_capacity != msg_nb
			|| head->max_length_message != max_message_length){
		printf("test_connexion() : Header attribute error\n");
		printf("Given name : %s.  Target pipe capacity : %d. Target max message length : %ld.\n",
				name, 12, max_message_length);
		printf("Actual name : %s.  Actual pipe capacity : %d. Actual max message length : %ld.\n",
				file->name, head->pipe_capacity, head->max_length_message);
		printf("\n");
		// return -1; // debug
	}

	// Verifie la memoire allouee
	if(file->memory_size != sizeof(header) + (sizeof(mon_message) + max_message_length) * msg_nb){
		printf("test_connexion : Memory allocation error\n");
		printf("Target memory size : %ld\n", sizeof(header) + (sizeof(mon_message) + max_message_length) * msg_nb);
		printf("Actual memory size : %ld\n", file->memory_size);
		printf("\n");
		return -1;
	}

	// Verifie l'initialisation des index du tableau de message
	if(head->first_free != 0 || head->last_free != 0 || head->first_occupied != -1 || head->last_occupied != -1){
		printf("test_connexion: Memory indexes error\n");
		printf("Target index values : 0, 0, -1, -1\n");
		printf("Actual index values: %d, %d, %d, %d\n",
				head->first_free, head->last_free, head->first_occupied, head->last_occupied);
		printf("\n");
		return -1;
	}

	// debug : Tester les droits (O_RDWR et S_IRWXU)
	// debug : tester les droits avec differentes valeurs
	// debug : tester la connexion quand existe deja + le rejet quand existe deja et O_EXECL

	printf("test_connexion() : OK\n\n");
	return 0;
}

int test_envoi(){
	int t[4] = {-12, 99, 134, 543};
	MESSAGE* file = m_connexion("/envoi", O_RDWR | O_CREAT, 1, sizeof(t), S_IRWXU | S_IRWXG | S_IRWXO);
	struct header *head = &file->shared_memory->head;

	struct mon_message *m = malloc( sizeof( struct mon_message ) + sizeof( t ) );
	if( m == NULL ){ perror("Function test malloc()"); exit(-1); }

	m->type = (long) getpid();
	memmove( m->mtext, t, sizeof( t ));

	// Debug : verifier les erreurs de m_envoi

	// Test : envoi en mode non bloquant AVEC de la place
	if( m_envoi( file, m, sizeof(t), O_NONBLOCK) != 0 ){
		printf("test_envoi() : echec envoie 1er message.\n");
		return -1;
	}

	// Verifie les valeurs des index
	if(head->first_free != -1 || head->last_free != -1
			|| head->first_occupied != 0 || head->last_occupied != 0){
		printf("test_envoi() : erreur d'indice apres envoi 1er message.\n");
		printf("Target index values : -1, -1, 0, 0\n");
		printf("Actual index values: %d, %d, %d, %d\n",
				head->first_free, head->last_free, head->first_occupied, head->last_occupied);
		printf("\n");
		return -1;
	}

	// Test : envoi en mode non bloquant SANS place
	if( m_envoi( file, m, sizeof(t), O_NONBLOCK) != -1 &&  errno != EAGAIN){
		printf("test_envoi() : echec gestion envoi dans un tableau plein (mode non bloquant).\n");
		// debug : details to add
		printf("\n");
		return -1;
	}

	printf("test_envoi() : OK\n\n");
	return 0;
}


int test_envois_multiples(){
	int t[4] = {-12, 99, 134, 543};
	char name[] = "/test_envois_multiples";
	int msg_nb = 9; // debug : segmentation fault quand on passe de 9 à 10
	size_t max_message_length = sizeof(t);
	size_t size_msg = sizeof( struct mon_message ) + sizeof( t );

	MESSAGE* file = m_connexion(name, O_RDWR | O_CREAT, msg_nb, max_message_length, S_IRWXU | S_IRWXG | S_IRWXO);
	struct header *head = &file->shared_memory->head;

	struct mon_message *m = malloc(size_msg);
	if( m == NULL ){ perror("Function test malloc()"); exit(-1); }

	m->type = (long) getpid();
	memmove( m->mtext, t, sizeof( t ));

	// Test : envois en mode non bloquant AVEC de la place
	for(int j = 0; j < msg_nb; j++){
		if( m_envoi( file, m, sizeof(t), O_NONBLOCK) != 0 ){
			printf("test_envoi() : echec envoie message n %d.\n", j + 1);
			return -1;
		}

		// Verifie les valeurs des index tant que le tableau n'est pas plein
		if(size_msg * (j+2) <= file->memory_size - sizeof(header)){
			if(head->first_free != size_msg * (j+1) || head->last_free != size_msg * (j+1)
					|| head->first_occupied != 0 || head->last_occupied != size_msg * (j)){
				printf("test_envoi() : erreur d'indice apres envoi message n %d.\n", j + 1);
				printf("Target index values : -1, -1, 0, %ld\n", size_msg * (j));
				printf("Actual index values: %d, %d, %d, %d\n",
						head->first_free, head->last_free, head->first_occupied, head->last_occupied);
				printf("\n");
				return -1;
			}
		}
		else{
			if(head->first_free != -1 || head->last_free != -1
					|| head->first_occupied != 0 || head->last_occupied != size_msg * (j)){
				printf("test_envoi() : erreur d'indice apres envoi message n %d.\n", j + 1);
				printf("Target index values : -1, -1, 0, %ld\n", size_msg * (j));
				printf("Actual index values: %d, %d, %d, %d\n",
						head->first_free, head->last_free, head->first_occupied, head->last_occupied);
				printf("\n");
				return -1;
			}
		}
	}

	// Test : envoi en mode non bloquant SANS place
	if( m_envoi( file, m, sizeof(t), O_NONBLOCK) != -1 &&  errno != EAGAIN){
		printf("test_envoi() : echec gestion envoi dans un tableau plein (mode non bloquant).\n");
		// debug : details to add
		printf("\n");
		return -1;
	}

	printf("test_envois_multiples() : OK\n\n");
	return 0;
}




int main(int argc, const char * argv[]) {
	printf("\n\n");
	test_connexion();
	test_envoi();
	test_envois_multiples();
	/*
	MESSAGE* file = m_connexion("/kangourou", O_RDWR | O_CREAT, 12, sizeof(char)*20, S_IRWXU | S_IRWXG | S_IRWXO);
	struct header *head = &file->shared_memory->head;

	int t[4] = {-12, 99, 134, 543}; //valeurs à envoyer

	struct mon_message *m = malloc( sizeof( struct mon_message ) + sizeof( t ) );
	if( m == NULL ){ perror("Function test malloc()"); exit(-1); }
	m->type = (long) getpid(); // comme type de message, on choisit l’identité de l’expéditeur
	memmove( m -> mtext, t, sizeof( t )) ; //copier les deux int à envoyer


	printf("taille de mon_message : %ld\n", sizeof(mon_message)); // debug
	printf("taille du tableau : %ld\n", sizeof(t)); // debug
	printf("taille d'un message complet : %ld\n", sizeof( struct mon_message ) + sizeof( t )); // debug
	printf("\n");

	for(int j = 0; j < 10; j++){
		printf("iteration n%d \n", j + 1);
		//printf("type envoye : %ld \n", m->type);

		// envoyer en mode non bloquant
		int i = m_envoi( file, m, sizeof( t ), O_NONBLOCK) ;

		if( i == 0 ){ printf("message envoye.\n"); }
		else if( i == -1 && errno == EAGAIN ){ printf("file pleine, reessayer plus tard"); }
		else{ perror("erreur menvoi()"); exit(-1); }

		// DEBUG
		printf("first free : %d\n", head->first_free);
		printf("last free : %d\n", head->last_free);
		printf("first occupied : %d\n", head->first_occupied);
		printf("last occupied : %d\n", head->last_occupied);

		printf("total memory for messages : %ld\n", file->memory_size - sizeof(header));
		printf("taille d'un message complet : %ld\n", sizeof( struct mon_message ) + sizeof( t ));
		printf("memoire utilisee : %ld\n", (sizeof( struct mon_message ) + sizeof( t )) * (j+1));
		printf("memoire restante : %ld\n", file->memory_size - sizeof(header) - (sizeof( struct mon_message ) + sizeof( t )) * (j+1));
		printf("\n\n");
		// FIN DEBUG
	}
	*/
	/*
	// reception
	struct mon_message *m2 = malloc( sizeof( struct mon_message ) + sizeof( t ) );
	if( m == NULL ){ perror("Function test malloc()"); exit(-1); }

	ssize_t s = m_reception(file, m2, sizeof( struct mon_message ) + sizeof( t ), 0, 0);

	if( s > 0 ){
		printf("message recu.\n");
		printf("La valeur du type est %ld.\n", m2->type);
		printf("Le msg est %s.\n", m2->mtext);
	}
	else{ perror("erreur m_reception()"); exit(-1); }
	*/

	return EXIT_SUCCESS;
}
