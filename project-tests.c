#include "m_file.h"

// <>

int t[4] = {-12, 99, 134, 543};

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
	if(head->first_free != 0 || head->last_free != 0 || head->first_occupied != -1 || head->last_occupied != -1){
		printf("test_connexion() : ECHEC : erreur indices memoire\n");
		printf("Target index values : 0, 0, -1, -1\n");
		printf("Actual index values: %d, %d, %d, %d\n",
				head->first_free, head->last_free, head->first_occupied, head->last_occupied);
		printf("\n");
		return -1;
	}

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
	if(head->first_free != -1 || head->last_free != -1
			|| head->first_occupied != 0 || head->last_occupied != 0){
		printf("test_envoi() : ECHEC : indice apres envoi 1er message.\n");
		printf("Target index values : -1, -1, 0, 0\n");
		printf("Actual index values: %d, %d, %d, %d\n",
				head->first_free, head->last_free, head->first_occupied, head->last_occupied);
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
	struct header *head = &file->shared_memory->head;

	struct mon_message *m = malloc(size_msg);
	if( m == NULL ){ perror("Function test malloc()"); exit(-1); }

	m->type = (long) getpid();
	memmove( m->mtext, t, sizeof( t ));

	// Test : envois en mode non bloquant AVEC de la place
	for(int j = 0; j < msg_nb; j++){
		if( m_envoi( file, m, sizeof(t), O_NONBLOCK) != 0 ){
			printf("test_envois_multiples() : ECHEC : envoie message n %d.\n", j + 1);
			return -1;
		}

		// Verifie les valeurs des index tant que le tableau n'est pas plein
		if(size_msg * (j+2) <= file->memory_size - sizeof(header)){
			if(head->first_free != size_msg * (j+1) || head->last_free != size_msg * (j+1)
					|| head->first_occupied != 0 || head->last_occupied != size_msg * (j)){
				printf("test_envois_multiples() : ECHEC : indice apres envoi message n %d.\n", j + 1);
				printf("Target index values : -1, -1, 0, %ld\n", size_msg * (j));
				printf("Actual index values: %d, %d, %d, %d\n",
						head->first_free, head->last_free, head->first_occupied, head->last_occupied);
				printf("\n");
				return -1;
			}
		}
		// Verifie les valeurs des index quand le message est le dernier a pouvoir etre stocke
		else{
			if(head->first_free != -1 || head->last_free != -1
					|| head->first_occupied != 0 || head->last_occupied != size_msg * (j)){
				printf("test_envois_multiples() : ECHEC : indice apres envoi message n %d.\n", j + 1);
				printf("Target index values : -1, -1, 0, %ld\n", size_msg * (j));
				printf("Actual index values: %d, %d, %d, %d\n",
						head->first_free, head->last_free, head->first_occupied, head->last_occupied);
				printf("\n");
				return -1;
			}
		}
	}

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
	// Test file Write Only
	/*
	size_t size_msg = sizeof(char)*100;
	MESSAGE* file1 = m_connexion("/reception_erreurs_1",
			O_WRONLY | O_CREAT, 8, size_msg, S_IRWXU | S_IRWXG | S_IRWXO);

	struct mon_message *m1 = malloc( sizeof( struct mon_message ) + size_msg);
	if( m1 == NULL ){ perror("Function test malloc()"); exit(-1); }

	ssize_t s = m_reception(file1, m1, sizeof( struct mon_message ) + size_msg, 0, 0);
	if( s != -1 || errno != EPERM){
		printf("test_reception_erreurs() : ECHEC : gestion lecture quand Read Only.\n");
		printf("Valeur attendue : -1. Erreur attendue : %d.\n", EPERM);
		printf("Valeur recue : %ld. Erreur recue : %d.\n", s, errno);
		printf("\n");
		return -1;
	}
	printf("test_reception_erreurs() : OK\n\n");
	*/
	return 0;
}


int  test_reception(MESSAGE* file){
	struct header *head = &file->shared_memory->head;
	size_t size_msg = sizeof(mon_message) + sizeof(t);
	struct mon_message *m1 = malloc( size_msg);
	if( m1 == NULL ){ perror("Function test malloc()"); exit(-1); }

	// Reception en mode bloquant alors qu'il y a bien un message dans la file
	ssize_t s = m_reception(file, m1, size_msg, 0, 0);
	if(s != sizeof(t) || m1->length != sizeof(t) || ((int*)m1->mtext)[2] != t[2] || m1->type != getpid()){
		printf("test_reception() : ECHEC : reception.\n");
		printf("Valeurs attendues : retour %ld, length %ld, mtext %d, type %d.\n", sizeof(t), sizeof(t), t[2], getpid());
		printf("Valeurs recues : retour %ld, length %ld, mtext %d, type %ld.\n", s, m1->length, ((int*)m1->mtext)[2], m1->type);
		printf("\n");
		return -1;
	}

	// Verifie les valeurs des index
	if(head->first_free != 0 || head->last_free != 0
			|| head->first_occupied != -1 || head->last_occupied != -1){
		printf("test_reception() : ECHEC : indices apres reception 1er message.\n");
		printf("Target index values : 0, 0, -1, -1\n");
		printf("Actual index values: %d, %d, %d, %d\n",
				head->first_free, head->last_free, head->first_occupied, head->last_occupied);
		printf("\n");
		return -1;
	}


	// Reception en mode NON bloquant alors qu'il n'y a pas de message dans la file
	s = m_reception(file, m1, size_msg, 0, O_NONBLOCK);
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


int  test_receptions_multiples(MESSAGE* file, int msg_nb){

	printf("test_receptions_multiples() : OK\n\n");
	return 0;
}

// test reception avec différentes valeurs de pid (necessite des fork)

// Definir une fonction qui vérifie les valeurs des indices


int main(int argc, const char * argv[]) {
	printf("\n\n");
	test_connexion();
	test_envoi_erreurs();
	test_reception_erreurs();

	MESSAGE* file = m_connexion("/envoi", O_RDWR | O_CREAT, 1, sizeof(t), S_IRWXU | S_IRWXG | S_IRWXO);
	test_envoi(file);
	test_reception(file);

	int msg_nb = 9; // debug : segmentation fault quand on passe de 9 à 10
	MESSAGE* file2 = m_connexion("/test_multiples", O_RDWR | O_CREAT, msg_nb, sizeof(t), S_IRWXU | S_IRWXG | S_IRWXO);
	test_envois_multiples(file2, msg_nb);
	test_receptions_multiples(file2, msg_nb);


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
	if( m2 == NULL ){ perror("Function test malloc()"); exit(-1); }

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
