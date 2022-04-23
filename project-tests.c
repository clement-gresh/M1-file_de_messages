#include "m_file.h"

// <>

void test_connexion(){
	char name[] = "/kangourou";
	MESSAGE* file = m_connexion(name, O_RDWR | O_CREAT, 12, sizeof(char)*20, S_IRWXU | S_IRWXG | S_IRWXO);
	struct header *head = &file->shared_memory->head;

	if(strcmp(file->name, name) != 0 || head->pipe_capacity != 12 || head->max_length_message != sizeof(char)*20){
		printf("test_connexion() : Header attribute error\n");
		printf("Given name : %s.  Target pipe capacity : %d. Target max message length : %ld.\n",
				"/kangourou", 12, sizeof(char)*20);
		printf("Actual name : %s.  Actual pipe capacity : %d. Actual max message length : %ld.\n",
				file->name, head->pipe_capacity, head->max_length_message);
		printf("\n");
	}

	if(file->memory_size != sizeof(header) + (sizeof(mon_message) + 20 * sizeof(char)) * 12){
		printf("test_connexion : Memory allocation error\n");
		printf("Target memory size : %ld\n", sizeof(header) + (sizeof(mon_message) + 20 * sizeof(char)) * 12);
		printf("Actual memory size : %ld\n", file->memory_size);
		printf("\n");
	}

	if(head->first_free != 0 || head->last_free != 0 || head->first_occupied != -1 || head->last_occupied != -1){
		printf("test_connexion: Memory indexes error\n");
		printf("Target index values : 0, 0, -1, -1\n");
		printf("Actual index values: %d, %d, %d, %d\n",
				head->first_free, head->last_free, head->first_occupied, head->last_occupied);
		printf("\n");
	}

	// debug : Tester les droits (O_RDWR et S_IRWXU)
	// debug : tester les droits avec differentes valeurs
	// debug : tester la connexion quand existe deja + le rejet quand existe deja et O_EXECL
}

int test_envoi_reception(){
	int t[4] = {-12, 99, 134, 543};
	MESSAGE* file = m_connexion("/envoi_reception", O_RDWR | O_CREAT, 1, sizeof(t), S_IRWXU | S_IRWXG | S_IRWXO);
	struct header *head = &file->shared_memory->head;

	struct mon_message *m = malloc( sizeof( struct mon_message ) + sizeof( t ) );
	if( m == NULL ){ perror("Function test malloc()"); exit(-1); }

	m->type = (long) getpid();
	memmove( m->mtext, t, sizeof( t ));

	// Debug : verifier les erreurs de m_envoi

	// envoi en mode non bloquant
	if( m_envoi( file, m, sizeof(t), O_NONBLOCK) != 0 ){
		printf("test_envoi_reception() : echec envoie 1er message.\n");
		return -1;
	}

	//int msg_size = sizeof(mon_message) + sizeof(t);
	if(head->first_free != -1 || head->last_free != -1
			|| head->first_occupied != 0 || head->last_occupied != 0){
		printf("test_envoi_reception : erreur d'indice apres envoi 1er message.\n");
		printf("Target index values : -1, -1, 0, 0\n");
		printf("Actual index values: %d, %d, %d, %d\n",
				head->first_free, head->last_free, head->first_occupied, head->last_occupied);
		printf("\n");
		return -1;
	}

	if( m_envoi( file, m, sizeof(t), O_NONBLOCK) != -1 &&  errno != EAGAIN){
			printf("test_envoi_reception() : echec gestion envoi dans un tableau plein (mode non bloquant).\n");
			// debug : details to add
			printf("\n");
			return -1;
		}

	printf("test_envoi_reception() : OK\n\n");
	return 0;
}

int main(int argc, const char * argv[]) {
	printf("\n\n");
	test_connexion();
	test_envoi_reception();

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

	for(int i = 0; i < 10; i++){
		printf("iteration n%d \n", i + 1);
		printf("type envoye : %ld \n", m->type);

		// envoyer en mode non bloquant
		int i = m_envoi( file, m, sizeof( t ), O_NONBLOCK) ;

		if( i == 0 ){ printf("message envoye.\n"); }
		else if( i == -1 && errno == EAGAIN ){ printf("file pleine, reessayer plus tard"); }
		else{ perror("erreur menvoi()"); exit(-1); }

		// DEBUG
		printf("\n");
		printf("first free : %d\n", head->first_free);
		printf("last free : %d\n", head->last_free);
		printf("first occupied : %d\n", head->first_occupied);
		printf("last occupied : %d\n", head->last_occupied);
		printf("\n\n");
		// FIN DEBUG
	}

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
