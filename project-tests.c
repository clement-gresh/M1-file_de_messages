#include "m_file.h"


int main(int argc, const char * argv[]) {

	MESSAGE* file = m_connexion("/kangourou", O_RDWR | O_CREAT, 12, sizeof(char)*50, S_IRWXU | S_IRWXG | S_IRWXO);
	//printf("%s\n", &file->name);


	int t[2] = {-12, 99}; //valeurs à envoyer

	struct mon_message *m = malloc( sizeof( struct mon_message ) + sizeof( t ) );
	if( m == NULL ){ perror("Function test malloc()"); exit(-1); }

	m->type = (long) getpid(); // comme type de message, on choisit l’identité de l’expéditeur
	memmove( m -> mtext, t, sizeof( t )) ; //copier les deux int à envoyer

	// envoyer en mode non bloquant
	int i = m_envoi( file, m, sizeof( t ), O_NONBLOCK) ;

	if( i == 0 ){ printf("message envoye.\n"); }
	else if( i == -1 && errno == EAGAIN ){ printf("file pleine, reessayer plus tard"); }
	else{ perror("erreur menvoi()"); exit(-1); }


	return EXIT_SUCCESS;
}
