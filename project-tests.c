#include "m_file.h"

// <>

int main(int argc, const char * argv[]) {

	MESSAGE* file = m_connexion("/kangourou", O_RDWR | O_CREAT, 12, sizeof(char)*20, S_IRWXU | S_IRWXG | S_IRWXO);
	//printf("%s\n", &file->name);


	// DEBUG
	printf("\n\n");
	printf("name : %s\n", file->name);
	printf("pipe capacity : %d\n", file->shared_memory->head.pipe_capacity);
	printf("max length message : %d\n", file->shared_memory->head.max_length_message);
	printf("header size : %ld\n", sizeof(header));
	printf("memory size : %ld\n", file->memory_size);
	printf("Target memory size : %ld\n",
		(sizeof(header) + file->shared_memory->head.max_length_message) * file->shared_memory->head.pipe_capacity);
	printf("\n");
	printf("first free : %d\n", file->shared_memory->head.first_free);
	printf("last free : %d\n", file->shared_memory->head.last_free);
	printf("first occupied : %d\n", file->shared_memory->head.first_occupied);
	printf("last occupied : %d\n", file->shared_memory->head.last_occupied);
	printf("\n\n");
	// FIN DEBUG

	int t[2] = {-12, 99}; //valeurs à envoyer

	struct mon_message *m = malloc( sizeof( struct mon_message ) + sizeof( t ) );
	if( m == NULL ){ perror("Function test malloc()"); exit(-1); }
	m->type = (long) getpid(); // comme type de message, on choisit l’identité de l’expéditeur
	memmove( m -> mtext, t, sizeof( t )) ; //copier les deux int à envoyer

	for(int i = 0; i < 10; i++){
		printf("type envoye : %ld \n", m->type);

		// envoyer en mode non bloquant
		int i = m_envoi( file, m, sizeof( t ), O_NONBLOCK) ;

		if( i == 0 ){ printf("message envoye.\n"); }
		else if( i == -1 && errno == EAGAIN ){ printf("file pleine, reessayer plus tard"); }
		else{ perror("erreur menvoi()"); exit(-1); }
		printf("\n\n"); // debug
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
