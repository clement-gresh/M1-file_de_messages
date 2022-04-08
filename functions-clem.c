#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <fcntl.h>

#include "m_file.h"

// <>

int m_envoi(MESSAGE *file, const void *msg, size_t len, int msgflag){
	// ERREURS
	if(file->flag == O_RDONLY){
		printf("Impossible d'ecrire dans cette file.\n");
		errno = EPERM;
		exit(-1);
	}
	if(len > file->shared_memory->head.max_length_message){
		printf("La taille du message excede la taille maximale autorisee.\n");
		errno = EMSGSIZE;
		exit(-1);
	}
	if(msgflag != 0 && msgflag != O_NONBLOCK){
		printf("Valeur de msgflag incorrecte.\n");
		errno = EIO;
		exit(-1);
	}
	if((file->shared_memory->head.first == file->shared_memory->head.last) && (msgflag & O_NONBLOCK)){
		printf("Le tableau est plein (envoi en mode non bloquant).\n");
		errno = EAGAIN;
		exit(-1);
	}
	//Pas besoin de verifier que last + len < first car c'est un tableau, chaque message prend une case
	//Ainsi, si le tableau est plein, last = first

	// ECRITURE
	file->shared_memory->messages[file->shared_memory->head.last].mtext = msg;
	// Incremente last modulo la taille du tableau de messages
	file->shared_memory->head.last = (file->shared_memory->head.last + 1) % file->shared_memory->head.pipe_capacity;


	return 0; // debug : ligne a changer
}


int main(int argc, const char * argv[]) {

	return EXIT_SUCCESS;
}


