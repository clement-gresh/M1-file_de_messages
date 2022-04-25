#include "m_file.h"

// <>

int t[4] = {-12, 99, 134, 543};


// Verifie que les index sont egaux aux valeurs des parametres
int index_check(header * head, char text[], int first_free, int last_free, int first_occupied, int last_occupied){

	if(head->first_free != first_free || head->last_free != last_free
			|| head->first_occupied != first_occupied || head->last_occupied != last_occupied){

		printf("%s", text);
		printf("Indices cibles : %d, %d, %d, %d\n", first_free, last_free, first_occupied, last_occupied);
		printf("Indices reels : %d, %d, %d, %d\n",
				head->first_free, head->last_free, head->first_occupied, head->last_occupied);
		printf("\n");
		return -1;
	}
	return 0;
}



int test_connexion(){
	char name[] = "/kangourouqkdjnqkdjfnqlfn";
	int msg_nb = 12;
	size_t max_message_length = sizeof(char)*20;
	MESSAGE* file = m_connexion(name, O_RDWR | O_CREAT, msg_nb, max_message_length, S_IRWXU | S_IRWXG | S_IRWXO);
	struct header *head = &file->shared_memory->head;

	// Verifie dans la file les attributs nom, nombre de messages et longueur max d'un message
	if(strcmp(file->name, name) != 0 || head->pipe_capacity != msg_nb
			|| head->max_length_message != max_message_length){
		printf("test_connexion() : ECHEC : Header attribute error\n");
		printf("Given name : %s.  Target pipe capacity : %d. Target max message length : %ld.\n",
				name, 12, max_message_length);
		printf("Actual name : %s.  Actual pipe capacity : %d. Actual max message length : %ld.\n",
				file->name, head->pipe_capacity, head->max_length_message);
		printf("\n");
		// return -1; // debug
	}

	// Verifie la memoire allouee
	if(file->memory_size != sizeof(header) + (sizeof(mon_message) + max_message_length) * msg_nb){
		printf("test_connexion() : ECHEC : erreur allocation memoire\n");
		printf("Target memory size : %ld\n", sizeof(header) + (sizeof(mon_message) + max_message_length) * msg_nb);
		printf("Actual memory size : %ld\n", file->memory_size);
		printf("\n");
		return -1;
	}

	// Verifie l'initialisation des index du tableau de message
	if(index_check(head, "test_connexion() : ECHEC : erreur indices memoire\n", 0, 0, -1, -1) == -1) { return -1; };

	// debug : Tester les droits (O_RDWR et S_IRWXU)
	// debug : tester les droits avec differentes valeurs
	// debug : tester la connexion quand existe deja + le rejet quand existe deja et O_EXECL
	// debug : tester la connexion a une file anonyme par un processus enfant

	printf("test_connexion() : OK\n\n");
	return 0;
}


int test_envoi_erreurs(){
	struct mon_message *m = malloc( sizeof( struct mon_message ) + sizeof( t ) );
	if( m == NULL ){ perror("Function test malloc()"); exit(-1); }
	m->type = (long) getpid();
	memmove( m->mtext, t, sizeof( t ));

	/*
	// Test file Read Only
	MESSAGE* file1 = m_connexion("/envoi_erreurs_1",
		O_RDONLY | O_CREAT, 8, sizeof(t)*10, S_IRWXU | S_IRWXG | S_IRWXO);

	int i = m_envoi( file1, m, sizeof(t), O_NONBLOCK);
	if( i != -2 || errno != EPERM){
		printf("test_envoi_erreurs() : ECHEC : gestion envoi dans un tableau read only.\n");
		printf("Valeur attendue : -1. Erreur attendue : %d.\n", EAGAIN);
		printf("Valeur recue : %d. Erreur recue : %d.\n", i, errno);
		printf("\n");
		return -1;
	}
	*/

	// Test envoi d'un message trop long
	size_t small_size = sizeof(t) - sizeof(char);
	MESSAGE* file2 = m_connexion("/envoi_erreurs_2", O_RDWR | O_CREAT, 5, small_size, S_IRWXU | S_IRWXG | S_IRWXO);

	int i = m_envoi( file2, m, sizeof(t), O_NONBLOCK);
	if( i != -1 || errno != EMSGSIZE){
		printf("test_envoi_erreurs() : ECHEC : gestion envoi d'un message trop long.\n");
		printf("Valeur attendue : -1. Erreur attendue : %d.\n", EMSGSIZE);
		printf("Valeur recue : %d. Erreur recue : %d.\n", i, errno);
		printf("\n");
		return -1;
	}

	// Test envoi avec mauvai drapeau
	MESSAGE* file3 = m_connexion("/envoi_erreurs_3", O_RDWR | O_CREAT, 5, sizeof(t), S_IRWXU | S_IRWXG | S_IRWXO);

	i = m_envoi( file3, m, sizeof(t), O_RDWR);
	if( i != -1 || errno != EIO){
		printf("test_envoi_erreurs() : ECHEC : gestion envoi avec mauvais drapeau.\n");
		printf("Valeur attendue : -1. Erreur attendue : %d.\n", EIO);
		printf("Valeur recue : %d. Erreur recue : %d.\n", i, errno);
		printf("\n");
		return -1;
	}

	printf("test_envoi_erreurs() : OK\n\n");
	return 0;
}

int test_envoi(MESSAGE* file){
	struct header *head = &file->shared_memory->head;
	mon_message *messages = (mon_message *)file->shared_memory->messages;

	struct mon_message *m = malloc( sizeof( struct mon_message ) + sizeof( t ) );
	if( m == NULL ){ perror("Function test malloc()"); exit(-1); }

	m->type = (long) getpid();
	memmove( m->mtext, t, sizeof( t ));

	// Debug : verifier les erreurs de m_envoi

	// Test : envoi en mode non bloquant AVEC de la place
	if( m_envoi( file, m, sizeof(t), O_NONBLOCK) != 0 ){
		printf("test_envoi() : ECHEC : envoie 1er message.\n");
		return -1;
	}

	// Verifie les valeurs des index
	if(index_check(head, "test_envoi() : ECHEC : indice.\n", -1, -1, 0, 0) == -1) { return -1; };

	// Verifie la valeur de l'offset
	if(messages[0].offset != 0){
		printf("test_envoi() : ECHEC : offset message.\n");
		printf("offset voulu : %d\n", 0);
		printf("offset reel : %ld\n", messages[0].offset);
		printf("\n");
		return -1;
	}

	// Test : envoi en mode non bloquant SANS place
	int i = m_envoi( file, m, sizeof(t), O_NONBLOCK);
	if( i != -1 || errno != EAGAIN){
		printf("test_envoi() : ECHEC : gestion envoi dans un tableau plein (mode non bloquant).\n");
		printf("Valeur attendue : -1. Erreur attendue : %d.\n", EAGAIN);
		printf("Valeur recue : %d. Erreur recue : %d.\n", i, errno);
		printf("\n");
		return -1;
	}

	printf("test_envoi() : OK\n\n");
	return 0;
}

int test_envois_multiples(MESSAGE* file, int msg_nb){
	size_t size_msg = sizeof( struct mon_message ) + sizeof( t );
	mon_message *messages = (mon_message *)file->shared_memory->messages;
	header *head = &file->shared_memory->head;

	struct mon_message *m = malloc(size_msg);
	if( m == NULL ){ perror("Function test malloc()"); exit(-1); }

	memmove( m->mtext, t, sizeof( t ));

	// Test : envois en mode non bloquant AVEC de la place
	for(int j = 0; j < msg_nb; j++){
		m->type = (long) 1000 + pow(-2.0,(double) j);
		if( m_envoi( file, m, sizeof(t), O_NONBLOCK) != 0 ){
			printf("test_envois_multiples() : ECHEC : envoie message n %d.\n", j + 1);
			return -1;
		}

		// Verifie les valeurs des index tant que le tableau n'est pas plein
		if(size_msg * (j+2) <= file->memory_size - sizeof(header)){
			char text[] = "test_envois_multiples() : ECHEC : indice envoi multiple.\n";
			if(index_check(head, text, size_msg * (j+1), size_msg * (j+1), 0, size_msg * (j)) == -1) { return -1; };
		}
		// Verifie les valeurs des index quand le tableau est plein
		else{
			char text[] = "test_envois_multiples() : ECHEC : indice apres envoi dernier message.\n";
			if(index_check(head, text, -1, -1, 0, size_msg * (j)) == -1) { return -1; };
		}
	}
	// Verifie les offsets dans la file
	for(int j = 0; j < msg_nb; j++){
		if(j != msg_nb-1 && messages[j*size_msg].offset != size_msg){
			printf("test_envois_multiples() : ECHEC : offset.\n");
			printf("offset voulu pour mesage n %d : %ld\n", j+1, size_msg);
			printf("offset reel  pour mesage n %d : %ld\n", j+1, messages[j*size_msg].offset);
			printf("\n");
			return -1;
		}
		else if(j == msg_nb-1 && messages[j*size_msg].offset != 0){
			printf("test_envois_multiples() : ECHEC : offset dernier message.\n");
			printf("offset voulu pour dernier mesage (n %d) : %d\n", j+1, 0);
			printf("offset reel  pour dernier mesage (n %d) : %ld\n", j+1, messages[j*size_msg].offset);
			printf("\n");
			return -1;
		}
	}
	/*
	for(int j = 0; j < msg_nb; j++){
		printf("position message n %d : %ld\n", j, (long)( 1000 + pow(-2.0,(double) j)));
	}
	*/

	// Test : envoi en mode non bloquant SANS place
	for(int j = 0; j < 3; j++){
		int i = m_envoi( file, m, sizeof(t), O_NONBLOCK);
		if( i != -1 ||  errno != EAGAIN){
			printf("test_envois_multiples() : ECHEC : gestion envoi n %d dans un tableau plein (mode non bloquant).\n", j + 1);
			printf("Valeur attendue : -1. Erreur attendue : %d.\n", EAGAIN);
			printf("Valeur recue : %d. Erreur recue : %d.\n", i, errno);
			printf("\n");
			return -1;
		}
	}

	printf("test_envois_multiples() : OK\n\n");
	return 0;
}


int test_reception_erreurs(){
	// Test envoi avec drapeau incorrect
	MESSAGE* file = m_connexion("/reception_erreurs_1", O_RDWR | O_CREAT, 1, sizeof(t), S_IRWXU | S_IRWXG | S_IRWXO);

	struct mon_message *m1 = malloc( sizeof( struct mon_message ) + sizeof(t));
	if( m1 == NULL ){ perror("Function test malloc()"); exit(-1); }

	ssize_t s = m_reception(file, m1, sizeof(t), 0, O_RDWR);
	if( s != -1 || errno != EIO){
		printf("test_reception_erreurs() : ECHEC : gestion reception avec drapeau incorrect.\n");
		printf("Valeur attendue : -1. Erreur attendue : %d.\n", EIO);
		printf("Valeur recue : %ld. Erreur recue : %d.\n", s, errno);
		printf("\n");
		return -1;
	}

	// Test file Write Only
	/*
	size_t size_msg = sizeof(char)*100;
	MESSAGE* file1 = m_connexion("/reception_erreurs_2",
			O_WRONLY | O_CREAT, 8, size_msg, S_IRWXU | S_IRWXG | S_IRWXO);

	struct mon_message *m1 = malloc( sizeof( struct mon_message ) + size_msg);
	if( m1 == NULL ){ perror("Function test malloc()"); exit(-1); }

	ssize_t s = m_reception(file1, m1, size_msg, 0, 0);
	if( s != -1 || errno != EPERM){
		printf("test_reception_erreurs() : ECHEC : gestion lecture quand Read Only.\n");
		printf("Valeur attendue : -1. Erreur attendue : %d.\n", EPERM);
		printf("Valeur recue : %ld. Erreur recue : %d.\n", s, errno);
		printf("\n");
		return -1;
	}
	*/

	printf("test_reception_erreurs() : OK\n\n");
	return 0;
}


int test_reception(MESSAGE* file){
	struct header *head = &file->shared_memory->head;
	mon_message *messages = (mon_message *)file->shared_memory->messages;
	struct mon_message *m1 = malloc(sizeof(mon_message) + sizeof(t));
	if( m1 == NULL ){ perror("Function test malloc()"); exit(-1); }

	// Reception quand il y a bien un message dans la file
	ssize_t s = m_reception(file, m1, sizeof(t), 0, 0);
	if(s != sizeof(t) || m1->length != sizeof(t) || ((int*)m1->mtext)[2] != t[2] || m1->type != getpid()){
		printf("test_reception() : ECHEC : reception.\n");
		printf("Valeurs attendues : retour %ld, length %ld, mtext %d, type %d.\n", sizeof(t), sizeof(t), t[2], getpid());
		printf("Valeurs recues : retour %ld, length %ld, mtext %d, type %ld.\n", s, m1->length, ((int*)m1->mtext)[2], m1->type);
		printf("\n");
		return -1;
	}

	// Verifie les valeurs des index
	if(index_check(head, "test_reception() : ECHEC : indices apres reception 1er message.\n", 0, 0, -1, -1) == -1) { return -1; };

	// Verifie la valeur de l'offset
	if(messages[0].offset != 0){
		printf("test_reception() : ECHEC : offset message dans file de message (case vide).\n");
		printf("offset voulu : %d\n", 0);
		printf("offset reel : %ld\n", messages[0].offset);
		printf("\n");
		return -1;
	}


	// Reception en mode NON bloquant alors qu'il n'y a pas de message dans la file
	s = m_reception(file, m1, sizeof(t), 0, O_NONBLOCK);
	if( s != -1 || errno != EAGAIN){
		printf("test_reception() : ECHEC : gestion reception a partir d'un tableau vide (mode non bloquant).\n");
		printf("Valeur attendue : -1. Erreur attendue : %d.\n", EAGAIN);
		printf("Valeur recue : %ld. Erreur recue : %d.\n", s, errno);
		printf("\n");
		return -1;
	}

	printf("test_reception() : OK\n\n");
	return 0;
}


int test_receptions_multiples(MESSAGE* file, int msg_nb){
	size_t size_msg = sizeof( struct mon_message ) + sizeof( t );
	struct header *head = &file->shared_memory->head;
	mon_message *messages = (mon_message *)file->shared_memory->messages;
	struct mon_message *m1 = malloc(size_msg);
	if( m1 == NULL ){ perror("Function test malloc()"); exit(-1); }

	// Receptions multiples alors qu'il y a bien des messages dans la file
	for(int j = 0; j < msg_nb; j++){
		long type;
		if(j == 0){ type = 1004; }
		else if(j == 1){ type = -970; }
		else if(j == 4){ type = -2; }
		else{ type = (long) 1000 + pow(-2.0,(double) j); }

		ssize_t s = m_reception(file, m1, sizeof(t), type, O_NONBLOCK);
		// A remettre a BLOCK sauf pour ceux où ca ne marche pas et quelques autres ?

		// Reception avec un type specifique
		int position1 = 2;
		if(j == 0){
			if(s != sizeof(t) || m1->type != 1004){
				printf("test_receptions_multiples() : ECHEC : reception avec valeur specifique de type.\n");
				printf("Valeurs attendues : retour %ld, type %d.\n", sizeof(t), 1004);
				printf("Valeurs recues : retour %ld, type %ld.\n", s, m1->type);
				printf("\n");
				return -1;
			}

			// Verifie les valeurs des index
			char text[] = "test_receptions_multiples() : ECHEC : indices apres reception 1er message.\n";
			if(index_check(head, text, position1 * size_msg, position1 * size_msg, 0, (msg_nb - 1) * size_msg) == -1)
				{ return -1; };

			// Verifie la valeur de l'offset
			if(messages[head->first_free].offset != 0){
				printf("test_receptions_multiples() : ECHEC : offset case libre dans la file apres reception 1er msg.\n");
				printf("offset voulu (reception n %d) : %d\n", j+1, 0);
				printf("offset reel : %ld\n", messages[head->first_free].offset);
				printf("\n");
				return -1;
			}
		}
		// Reception avec type negatif
		else if(j == 1){
			int position2 = 5;
			if(s != sizeof(t) || m1->type != 968){
				printf("test_receptions_multiples() : ECHEC : reception avec valeur negative de type.\n");
				printf("Valeurs attendues : retour %ld, type %d.\n", sizeof(t), 968);
				printf("Valeurs recues : retour %ld, type %ld.\n", s, m1->type);
				printf("\n");
				return -1;
			}

			// Verifie les valeurs des index
			char text[] = "test_receptions_multiples() : ECHEC : indice apres reception 2eme message.\n";
			if(index_check(head, text, position1 * size_msg, position2 * size_msg, 0, (msg_nb - 1) * size_msg) == -1)
				{ return -1; };

			// Verifie la valeur de l'offset
			if(messages[head->first_free].offset != size_msg && messages[head->last_free].offset != 0){
				printf("test_receptions_multiples() : ECHEC : offset case libre dans la file apres reception 2eme msg.\n");
				printf("offset voulu (reception n %d) : case 1 : %ld, case 2 : %d\n",
						j+1, size_msg * (position2 - position1), 0);
				printf("offset reel : case 1 : %ld, case 2 : %ld\n",
						messages[head->first_free].offset, messages[head->last_free].offset);
				printf("\n");
				return -1;
			}
		}
		// Pas de messages correspondant au type demande (positif ou negatif), mode non bloquant
		else if(j == 2 || j == 5 || j == 4){
			if(s != -1 || errno != EAGAIN){
				printf("test_receptions_multiples() : ECHEC : gestion absence de message type, mode non bloquant.\n");
				printf("Valeurs attendues : retour %d, error %d.\n", -1, EAGAIN);
				printf("Valeurs recues : retour %ld, error %d.\n", s, errno);
				printf("\n");
				return -1;
			}
		}

		// Reception des autres messages
		else{
			if(s != sizeof(t) || m1->length != sizeof(t) || ((int*)m1->mtext)[2] != t[2]
						|| m1->type != (long) 1000 + pow(-2.0,(double) j)){
				printf("test_receptions_multiples() : ECHEC : reception multiples.\n");
				printf("Valeurs attendues : retour %ld, length %ld, mtext %d, type %f.\n",
						sizeof(t), sizeof(t), t[2], (long) 1000 + pow(-2.0,(double) j));
				printf("Valeurs recues : retour %ld, length %ld, mtext %d, type %ld.\n",
						s, m1->length, ((int*)m1->mtext)[2], m1->type);
				printf("\n");
				return -1;
			}
		}
	}
	printf("test_receptions_multiples() : OK\n\n");
	return 0;
}

int main(int argc, const char * argv[]) {
	printf("\n\n");
	test_connexion();
	test_envoi_erreurs();
	test_reception_erreurs();

	MESSAGE* file = m_connexion("/envoi", O_RDWR | O_CREAT, 1, sizeof(t), S_IRWXU | S_IRWXG | S_IRWXO);
	test_envoi(file);
	test_reception(file);

	int msg_nb = 11;
	MESSAGE* file2 = m_connexion("/test_multiples", O_RDWR | O_CREAT, msg_nb, sizeof(t), S_IRWXU | S_IRWXG | S_IRWXO);
	test_envois_multiples(file2, msg_nb);
	test_receptions_multiples(file2, msg_nb);

	return EXIT_SUCCESS;
}
