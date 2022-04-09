#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <pthread.h>
#include <sys/mman.h>

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

	// Lock du mutex
	if(pthread_mutex_lock(&file->shared_memory->head.mutex) != 0){
		perror("lock mutex");
		exit(-1);
	}

	// Ecrit le message dans la memoire partagee
	//file->shared_memory->messages[file->shared_memory->head.last] = &(struct mon_message*) msg;

	// DEBUG
	printf("La valeur du type est %ld.\n", file->shared_memory->messages[file->shared_memory->head.last].type);
	printf("Le msg est %s.\n", file->shared_memory->messages[file->shared_memory->head.last].mtext);
	// FIN DEBUG

	// Incremente 'last'
	file->shared_memory->head.last = (file->shared_memory->head.last + 1) % file->shared_memory->head.pipe_capacity;

	// Synchronise la memoire
	if(msync(file, sizeof(MESSAGE), MS_SYNC) == -1) {
		perror("Function msync()");
		exit(-1);
	}

	// Unlock le mutex
	if(pthread_mutex_unlock(&file->shared_memory->head.mutex) != 0){
		perror("UNlock mutex");
		exit(-1);
	}

	return 0;
}


int main(int argc, const char * argv[]) {

	return EXIT_SUCCESS;
}


