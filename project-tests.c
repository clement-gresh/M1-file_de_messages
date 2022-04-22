#include "m_file.h"

// <>

int main(int argc, const char * argv[]) {
	/*
	MESSAGE* file = m_connexion("/kangourou", O_RDWR | O_CREAT, 12, sizeof(char)*50, S_IRWXU | S_IRWXG | S_IRWXO);
	//printf("%s\n", &file->name);


	int t[2] = {-12, 99}; //valeurs à envoyer

	struct mon_message *m = malloc( sizeof( struct mon_message ) + sizeof( t ) );
	if( m == NULL ){ perror("Function test malloc()"); exit(-1); }

	m->type = (long) getpid(); // comme type de message, on choisit l’identité de l’expéditeur
	printf("type envoye : %ld \n", m->type);
	memmove( m -> mtext, t, sizeof( t )) ; //copier les deux int à envoyer

	// envoyer en mode non bloquant
	int i = m_envoi( file, m, sizeof( t ), O_NONBLOCK) ;

	if( i == 0 ){ printf("message envoye.\n"); }
	else if( i == -1 && errno == EAGAIN ){ printf("file pleine, reessayer plus tard"); }
	else{ perror("erreur menvoi()"); exit(-1); }


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
