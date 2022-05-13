#include "project-tests.h"

// <>

int t[4] = {-12, 99, 134, 543};

// Test deconnexion et destruction

// Ajouter test envois multiples apres receptions multiples

// plusieurs processus lancés en parallèle envoient et réceptionnent les messages

// Envoie puis recoit des messages petits pour verifier qu'on peut envoyer plus que nb_msg (memoire compacte)
// puis envoie des messages plus gros : verifier que la fonction fct_met_tout_a_gauche fonctionne

/*
int test_processus_paralleles(){
	// 2 proc creent / se connectent a une file
	// ils envoient 1000 messages
	// 2 proc en reçoivent 500 pendant que les 2 premiers continuent à en envoyer 500 en mode bloquant
	// il y a seulement 1000 places pour les messages

	printf("test_processus_paralleles() : OK\n");
	return 0;
}
*/

int main(int argc, const char * argv[]) {
	printf("\n");
	test_connexion();
	test_envoi_erreurs();
	test_reception_erreurs();

	MESSAGE* file = m_connexion("/envoi", O_RDWR | O_CREAT, 1, sizeof(t), S_IRWXU | S_IRWXG | S_IRWXO);
	test_envoi(file);
	test_reception(file);

	int msg_nb = 500; // msg_nb doit etre compris entre 7 (pour les tests) et 24 (segmentation fault car 'type' devient trop grand)
	MESSAGE* file2 = m_connexion("/test_multiples", O_RDWR | O_CREAT, msg_nb, sizeof(t), S_IRWXU | S_IRWXG | S_IRWXO);
	test_envois_multiples(file2, msg_nb);
	test_receptions_multiples(file2, msg_nb);
	test_compact_messages();

	return EXIT_SUCCESS;
}


// Verifie les valeurs du message recu (type, length, etc.)
int reception_check(mon_message *m1, char text[], ssize_t s, ssize_t size, size_t length, int value2, long type){
	if(s != size || m1->length != length || ((int*)m1->mtext)[2] != value2 || m1->type != type){
		printf("%s\n", text);
		printf("Valeurs attendues : retour %ld, length %ld, mtext %d, type %ld.\n", size, length, value2, type);
		printf("Valeurs recues : retour %ld, length %ld, mtext %d, type %ld.\n", s, m1->length, ((int*)m1->mtext)[2], m1->type);
		printf("\n");
		return -1;
	}
	return 0;
}

// Verifie que les index sont egaux aux valeurs des parametres
int index_check(MESSAGE* file, char text[], int first_free, int last_free, int first_occupied, int last_occupied){
	header *head = &file->shared_memory->head;

	if(head->first_free != first_free || head->last_free != last_free
			|| head->first_occupied != first_occupied || head->last_occupied != last_occupied){

		printf("%s\n", text);
		printf("Indices cibles : %d, %d, %d, %d\n", first_free, last_free, first_occupied, last_occupied);
		printf("Indices reels : %d, %d, %d, %d\n",
				head->first_free, head->last_free, head->first_occupied, head->last_occupied);
		printf("\n");
		return -1;
	}
	return 0;
}

// Verifie la valeur de l'offset d'une case de la file
int offset_check(MESSAGE* file, char text[], int offset, int position){
	if( ((mon_message *)&file->shared_memory->messages[position])->offset != offset ){
		printf("%s\n", text);
		printf("offset voulu : %d\n", offset);
		printf("offset reel : %ld\n", ((mon_message *)&file->shared_memory->messages[position])->offset);
		printf("\n");
		return -1;
	}
	return 0;
}

// Verifie les valeurs de retour suite a une erreur
int error_check(char text[], ssize_t s, int error){
	if(s != -1 || errno != error){
		printf("%s\n", text);
		printf("Valeurs attendues : retour %d, error %d.\n", -1, error);
		printf("Valeurs recues : retour %ld, error %d.\n", s, errno);
		printf("\n");
		return -1;
	}
	return 0;
}


// Teste la fonction m_connexion
int sous_test_connexion_1(){
	char name[] = "/nonorouqkdjnqkdjfnqlfn";
	int msg_nb = 12;
	size_t max_message_length = sizeof(char)*20;
	MESSAGE* file = m_connexion(name, O_RDWR | O_CREAT, msg_nb, max_message_length, S_IRWXU | S_IRWXG | S_IRWXO);
	if(file == NULL){
		printf("sous_test_connexion1() : ECHEC : NULL\n");
		return -1;
	}
	struct header *head = &file->shared_memory->head;

	// Verifie dans la file les attributs, nombre de messages et longueur max d'un message

	if(head->pipe_capacity != msg_nb){
		printf("Target pipe capacity : %d != %d.\n", head->pipe_capacity, msg_nb);
		printf("sous_test_connexion_1() : ECHEC : erreur pipe_capacity\n");
		return -1;
	}
	if(head->max_length_message != max_message_length){
		printf("Actual max message length : %ld != %ld.\n", head->max_length_message , max_message_length);
		printf("sous_test_connexion1() : ECHEC : erreur max_length_message\n");
		return -1;
	}
	// Verifie la memoire allouee
	size_t memory_size = sizeof(header) + (sizeof(mon_message) + max_message_length) * msg_nb;

	if(file->memory_size != memory_size){
		printf("sous_test_connexion_1() : ECHEC : erreur allocation memoire\n");
		printf("Target memory size : %ld\n",memory_size);
		printf("Actual memory size : %ld\n", file->memory_size);
		printf("\n");
		return -1;
	}

	// Verifie l'initialisation des index du tableau de message
	if(index_check(file, "sous_test_connexion_1() : ECHEC : erreur indices memoire.", 0, 0, -1, -1) == -1) { return -1; };

	printf("sous_test_connexion_1() : OK\n");
	return 0;
}

int sous_test_connexion_2(){
	char name[] = "/aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa";
	int msg_nb = 12;
	size_t max_message_length = sizeof(char)*20;
	MESSAGE* file = m_connexion(name, O_RDONLY | O_CREAT, msg_nb, max_message_length, 0);
	if(file != NULL){
		printf("sous_test_connexion2() : ECHEC : file != NULL\n");
		return -1;
	}
	
	printf("sous_test_connexion_2() : OK\n");
	return 0;
}

int sous_test_connexion_3(){
	char name[] = "/nonorouqkdjnqkdjfnqlfn";
	MESSAGE* file = m_connexion(name, O_RDONLY);
	if(file == NULL){
		printf("sous_test_connexion3() : ECHEC : NULL\n");
		return -1;
	}
	printf("sous_test_connexion3() : OK\n");
	return 0;
}

int sous_test_connexion_4(){
	char name[] = "/kangourou";
	int msg_nb = 12;
	size_t max_message_length = sizeof(char)*20;
	MESSAGE* file = m_connexion(name, O_RDWR | O_CREAT, msg_nb, max_message_length, S_IRWXU | S_IRWXG | S_IRWXO);
	if(file == NULL){
		printf("sous_test_connexion4() : ECHEC : NULL\n");
		return -1;
	}
	struct header *head = &file->shared_memory->head;

	// Verifie dans la file les attributs, nombre de messages et longueur max d'un message

	if(head->pipe_capacity != msg_nb){
		printf("Target pipe capacity : %d != %d.\n", head->pipe_capacity, msg_nb);
		printf("sous_test_connexion_4() : ECHEC : erreur pipe_capacity\n");
		return -1;
	}
	if(head->max_length_message != max_message_length){
		printf("Actual max message length : %ld != %ld.\n", head->max_length_message , max_message_length);
		printf("sous_test_connexion4() : ECHEC : erreur max_length_message\n");
		return -1;
	}
	// Verifie la memoire allouee
	size_t memory_size = sizeof(header) + (sizeof(mon_message) + max_message_length) * msg_nb;

	if(file->memory_size != memory_size){
		printf("sous_test_connexion_4() : ECHEC : erreur allocation memoire\n");
		printf("Target memory size : %ld\n", memory_size);
		printf("Actual memory size : %ld\n", file->memory_size);
		printf("\n");
		return -1;
	}

	// Verifie l'initialisation des index du tableau de message
	if(index_check(file, "sous_test_connexion_4() : ECHEC : erreur indices memoire.", 0, 0, -1, -1) == -1) { return -1; };

	if(fork()==0){ // enfant
		(file->memory_size)++;
		printf("Child started, value = %ld\n", file->memory_size);
		if(m_deconnexion(file)==-1){
			perror("m_deconnexion");
			return -1;
		}
		_exit(0);
	}
	else{
		if(wait(NULL)==-1){
			perror("wait");
			return -1;
		}
		printf("In parent, value = %ld\n", file->memory_size);
		if(m_deconnexion(file)==-1){
			perror("m_deconnexion");
			return -1;
		}
		printf("sous_test_connexion4() : OK\n");
		return 0;
	}
}

int sous_test_connexion_5(){
	char name[] = "/nonono";
	int msg_nb = 12;
	size_t max_message_length = sizeof(char)*20;
	MESSAGE* file = m_connexion(name, O_RDWR | O_CREAT | O_EXCL, msg_nb, max_message_length, S_IRWXU | S_IRWXG | S_IRWXO);
	if(file == NULL){
		printf("sous_test_connexion5() : ECHEC : NULL\n");
		return -1;
	}
	struct header *head = &file->shared_memory->head;

	// Verifie dans la file les attributs, nombre de messages et longueur max d'un message

	if(head->pipe_capacity != msg_nb){
		printf("Target pipe capacity : %d != %d.\n", head->pipe_capacity, msg_nb);
		printf("sous_test_connexion_5() : ECHEC : erreur pipe_capacity\n");
		return -1;
	}
	if(head->max_length_message != max_message_length){
		printf("Actual max message length : %ld != %ld.\n", head->max_length_message , max_message_length);
		printf("sous_test_connexion5() : ECHEC : erreur max_length_message\n");
		return -1;
	}
	// Verifie la memoire allouee
	if(file->memory_size != sizeof(header) + (sizeof(mon_message) + max_message_length) * msg_nb){
		printf("sous_test_connexion_5() : ECHEC : erreur allocation memoire\n");
		printf("Target memory size : %ld\n", sizeof(header) + (sizeof(mon_message) + max_message_length) * msg_nb);
		printf("Actual memory size : %ld\n", file->memory_size);
		printf("\n");
		return -1;
	}

	// Verifie l'initialisation des index du tableau de message
	if(index_check(file, "sous_test_connexion_5() : ECHEC : erreur indices memoire.", 0, 0, -1, -1) == -1) { return -1; };
	printf("sous_test_connexion_5() : OK\n");
	return 0;
}


int test_connexion(){
	sous_test_connexion_1(); //test connexion simple en O_RDWR | O_CREAT
	sous_test_connexion_2(); //test connexion simple en O_RDONLY | O_CREAT
	sous_test_connexion_3(); // test connexion sur un file existant
	sous_test_connexion_4(); //tester la connexion a une file anonyme par un processus enfant
	//sous_test_connexion_5(); //tester la connexion O_CREAT | O_EXCL test ok si on l'excécute 2 fois on a bien une erreur
	// debug lancer une exception et l'attraper pour pouvoir faire le test 5 ?

	// debug : Tester les droits (O_RDWR et S_IRWXU)
	// debug : tester les droits avec differentes valeurs

	printf("test_connexion() : OK\n\n");
	return 0;
}


// Teste la bonne gestion des erreurs dans m_envoi
int test_envoi_erreurs(){
	struct mon_message *m = malloc( sizeof( struct mon_message ) + sizeof( t ) );
	if( m == NULL ){ perror("Function test malloc()"); exit(-1); }
	m->type = (long) getpid();
	memmove( m->mtext, t, sizeof( t ));

	// Test envoi d'un message trop long
	size_t small_size = sizeof(t) - sizeof(char);
	MESSAGE* file2 = m_connexion("/envoi_erreurs_2", O_RDWR | O_CREAT, 5, small_size, S_IRWXU | S_IRWXG | S_IRWXO);
	int i = m_envoi( file2, m, sizeof(t), O_NONBLOCK);
	if(error_check("test_envoi_erreurs() : ECHEC : gestion envoi d'un message trop long.", i, EMSGSIZE) == -1) {return -1;}

	// Test envoi avec mauvais drapeau
	MESSAGE* file3 = m_connexion("/envoi_erreurs_3", O_RDWR | O_CREAT, 5, sizeof(t), S_IRWXU | S_IRWXG | S_IRWXO);
	i = m_envoi( file3, m, sizeof(t), O_RDWR);
	if(error_check("test_envoi_erreurs() : ECHEC : gestion envoi avec mauvais drapeau.", i, EIO) == -1) {return -1;}

	// Test file Read Only
	/*
	MESSAGE* file1 = m_connexion("/envoi_erreurs_3", O_RDONLY);
	i = m_envoi( file1, m, sizeof(t), O_NONBLOCK);
	char text[] = "test_envoi_erreurs() : ECHEC : gestion envoi dans un tableau read only.";
	if(error_check(text, i, EPERM) == -1) {return -1;}
	*/
	printf("test_envoi_erreurs() : OK\n\n");
	return 0;
}


// Teste m_envoi en envoyant un message quand la file est vide et un quand elle est pleine (mode non bloquant)
int test_envoi(MESSAGE* file){
	struct mon_message *m = malloc( sizeof( struct mon_message ) + sizeof( t ) );
	if( m == NULL ){ perror("Function test malloc()"); exit(-1); }

	m->type = (long) getpid();
	memmove( m->mtext, t, sizeof( t ));

	// Test : envoi en mode bloquant AVEC de la place
	if( m_envoi( file, m, sizeof(t), 0) != 0 ){
		printf("test_envoi() : ECHEC : envoie 1er message.\n"); return -1;
	}

	// Verifie les valeurs des index
	if(index_check(file, "test_envoi() : ECHEC : indice.", -1, -1, 0, 0) == -1) { return -1; };

	// Verifie la valeur de l'offset
	if(offset_check(file, "test_envoi() : ECHEC : offset message.", 0, 0) == -1) { return -1; };

	// Test : envoi en mode non bloquant SANS place
	int i = m_envoi( file, m, sizeof(t), O_NONBLOCK);
	if(error_check("test_envoi() : ECHEC : gestion envoi dans un tableau plein (mode non bloquant).", i, EAGAIN) == -1)
		{return -1;}

	printf("test_envoi() : OK\n\n");
	return 0;
}


// Test m_envoi en envoyant de multiples messages (file pleine et non pleine)
int test_envois_multiples(MESSAGE* file, int msg_nb){
	size_t size_msg = sizeof( struct mon_message ) + sizeof( t );
	struct mon_message *m = malloc(size_msg);
	if( m == NULL ){ perror("Function test malloc()"); exit(-1); }

	memmove( m->mtext, t, sizeof( t ));

	// Test : envois en mode non bloquant AVEC de la place
	for(int j = 0; j < msg_nb; j++){
		m->type = abs((long) 1000 + pow(-2.0,(double) j));
		if( m_envoi( file, m, sizeof(t), O_NONBLOCK) != 0 ){
			printf("test_envois_multiples() : ECHEC : envoie message n %d.\n", j + 1); return -1;
		}

		// Verifie les valeurs des index tant que le tableau n'est pas plein
		size_t free_memory = file->memory_size - sizeof(header);

		if(size_msg * (j+2) <= free_memory){
			char text[] = "test_envois_multiples() : ECHEC : indice envois multiples.";
			if(index_check(file, text, size_msg * (j+1), size_msg * (j+1), 0, size_msg * (j)) == -1) { return -1; };
		}
		// Verifie les valeurs des index quand le tableau est plein
		else{
			char text[] = "test_envois_multiples() : ECHEC : indice apres envoi dernier message.";
			if(index_check(file, text, -1, -1, 0, size_msg * (j)) == -1) { return -1; };
		}
	}
	// Verifie les offsets dans la file
	for(int j = 0; j < msg_nb; j++){
		if(j != msg_nb-1 && offset_check(file, "test_envois_multiples() : ECHEC : offset.", size_msg, j*size_msg) == -1)
			{ return -1; }

		else if(j == msg_nb-1
				&& offset_check(file, "test_envois_multiples() : ECHEC : offset dernier message.", 0, j*size_msg) == -1)
			{ return -1; }
	}

	// Test : envoi en mode non bloquant SANS place
	for(int j = 0; j < 3; j++){
		int i = m_envoi( file, m, sizeof(t), O_NONBLOCK);
		char text[] = "test_envois_multiples() : ECHEC : gestion envoi n dans un tableau plein (mode non bloquant).";

		if(error_check(text, i, EAGAIN) == -1) {return -1;}
	}

	printf("test_envois_multiples() : OK\n\n");
	return 0;
}


// Teste la bonne gestion des erreurs dans m_reception
int test_reception_erreurs(){
	struct mon_message *m1 = malloc( sizeof( struct mon_message ) + sizeof(t));

	// Test envoi avec drapeau incorrect
	MESSAGE* file = m_connexion("/reception_erreurs_1", O_RDWR | O_CREAT, 1, sizeof(t), S_IRWXU | S_IRWXG | S_IRWXO);
	if( m1 == NULL ){ perror("Function test malloc()"); exit(-1); }
	ssize_t s = m_reception(file, m1, sizeof(t), 0, O_RDWR);
	if(error_check("test_reception_erreurs() : ECHEC : gestion reception avec drapeau incorrect.", s, EIO) == -1)
		{return -1;}

	// Test file Write Only
	/*
	MESSAGE* file1 = m_connexion("/reception_erreurs_1", O_WRONLY);
	if( m1 == NULL ){ perror("Function test malloc()"); exit(-1); }

	s = m_reception(file1, m1, sizeof(t), 0, 0);
	if(error_check("test_reception_erreurs() : ECHEC : gestion lecture quand Write Only.", s, EPERM) == -1)
		{return -1;}
	*/

	// Test reception si la longueur de la memoire a l'adresse msg est plus petite que le message a lire : debug

	printf("test_reception_erreurs() : OK\n\n");
	return 0;
}


// Teste m_reception en tentant de recuperer un message de la file quand elle est pleine ou vide (mode non bloquant)
int test_reception(MESSAGE* file){
	struct mon_message *m1 = malloc(sizeof(mon_message) + sizeof(t));
	if( m1 == NULL ){ perror("Function test malloc()"); exit(-1); }

	// Reception quand il y a bien un message dans la file
	ssize_t s = m_reception(file, m1, sizeof(t), 0, 0);

	if(reception_check(m1, "test_reception() : ECHEC : reception.", s, sizeof(t), sizeof(t), t[2], getpid()) == -1)
		{return -1;}

	// Verifie les valeurs des index
	if(index_check(file, "test_reception() : ECHEC : indices apres reception 1er message.", 0, 0, -1, -1) == -1) {return -1;};

	// Verifie la valeur de l'offset dans la file
	if(offset_check(file, "test_reception() : ECHEC : offset case vide.", 0, 0) == -1) {return -1;}

	// Reception en mode NON bloquant alors qu'il n'y a pas de message dans la file
	s = m_reception(file, m1, sizeof(t), 0, O_NONBLOCK);
	if(error_check("test_reception() : ECHEC : gestion reception d'un tableau vide (mode non bloquant).", s, EAGAIN) == -1)
		{return -1;}

	printf("test_reception() : OK\n\n");
	return 0;
}


// Test m_reception en tentant de lire plusieurs messages en utilisant des valeurs positives et negatives de type
// Types dans la file : 1001, 998, 1004, 992, 1016, 968, 1064, 872, 1256, 488, 2024, 1048, 5096, 7192...
// Lit d'abord le message 1004 (type = 1004), puis 968 (type = -970)
// Puis lit dans l'ordre les cases suivantes a partir de 992
// Exception : le 5eme essai utilise type = -2 et donc echoue (la case 1016 n'est donc pas lue)
// A la fin les cases qui sont toujours occupees sont les cases 0 (1001), 1 (998), 4 (1016)
int test_receptions_multiples(MESSAGE* file, int msg_nb){
	size_t size_msg = sizeof( struct mon_message ) + sizeof( t );
	struct mon_message *m1 = malloc(size_msg);
	if( m1 == NULL ){ perror("Function test malloc()"); exit(-1); }
	int position1 = 2; // position of message with type 1004
	int position2 = 5; // position of message with type 968
	int position3 = 4; // position of message with type 1016

	// Teste reception avec un type strictement positif
	if(test_reception_type_pos(file, m1, size_msg, msg_nb, position1) == -1) {return -1;}

	// Teste reception avec un type strictement negatif
	if(test_reception_type_neg(file, m1, size_msg, msg_nb, position1, position2) == -1) {return -1;}

	// Receptions multiples alors qu'il y a bien des messages dans la file
	for(int j = 2; j < msg_nb; j++){
		long type;
		if(j == 4){ type = -2; }
		else{ type = abs((long) 1000 + pow(-2.0,(double) j)); }

		ssize_t s = m_reception(file, m1, sizeof(t), type, O_NONBLOCK);

		// Reception quand il n'y a pas de message de type demande (positif ou negatif), mode non bloquant
		if(j == position1 || j == position2 || j == position3){
			char text[] = "test_receptions_multiples() : ECHEC : gestion absence message type (non bloquant).";
			if(error_check(text, s, EAGAIN) == -1) {return -1;}
		}

		// Reception des autres messages
		else{
			// Verifie les donnees des messages
			char text0[] = "test_receptions_multiples() : ECHEC : reception multiples.";
			if(reception_check(m1, text0, s, sizeof(t), sizeof(t), t[2], type) == -1) {return -1;}

			// Verifie les offsets (case lue et case precedente)
			char text1[] = "test_receptions_multiples() : ECHEC : offset derniere case lue.";
			if(j > 6 && offset_check(file, text1, 0, size_msg * j) == -1) {return -1;}

			char text2[] = "test_receptions_multiples() : ECHEC : offset precedente case lue.";
			if(j > 6 && offset_check(file, text2, size_msg, size_msg * (j-1)) == -1) {return -1;}
		}
	}
	if(test_receptions_multiples_fin(file, msg_nb, size_msg, position1, position2, position3) == -1)
		{return -1;}

	printf("test_receptions_multiples() : OK\n\n");
	return 0;
}


// Teste la reception avec type strictement positif
int test_reception_type_pos(MESSAGE* file, mon_message *m1, size_t size_msg, int msg_nb, int position1){
	struct header *head = &file->shared_memory->head;
	char *messages = file->shared_memory->messages;

	ssize_t s = m_reception(file, m1, sizeof(t), ((mon_message *)&messages[position1*size_msg])->type, O_NONBLOCK);

	// Verifie les donnees du message
	char text0[] = "test_reception_type_pos() : ECHEC : reception avec valeur specifique de type.";
	if(reception_check(m1, text0, s, sizeof(t), sizeof(t), t[2], 1004) == -1)
		{return -1;}

	// Verifie les valeurs des index
	char text1[] = "test_reception_type_pos() : ECHEC : indices apres reception 1er message.";
	if(index_check(file, text1, position1 * size_msg, position1 * size_msg, 0, (msg_nb - 1) * size_msg) == -1)
		{ return -1; };

	// Verifie la valeur de l'offset
	char text2[] = "test_reception_type_pos() : ECHEC : offset case libre dans la file apres reception 1er msg.";
	if(offset_check(file, text2, 0, head->first_free) == -1)
		{return -1;}

	return 0;
}


// Teste la reception avec type negatif
int test_reception_type_neg(MESSAGE* file, mon_message *m1, size_t size_msg, int msg_nb, int position1, int position2){
	struct header *head = &file->shared_memory->head;
	char *messages = file->shared_memory->messages;

	ssize_t s = m_reception(file, m1, sizeof(t), -(((mon_message *)&messages[position2*size_msg])->type + 2), O_NONBLOCK);

	// Verifie les donnees du message
	char text0[] = "test_reception_type_neg() : ECHEC : reception avec valeur negative de type.";
	if(reception_check(m1, text0, s, sizeof(t), sizeof(t), t[2], 968) == -1) {return -1;}

	// Verifie les valeurs des index
	char text[] = "test_reception_type_neg() : ECHEC : indice apres reception 2eme message.";
	if(index_check(file, text, position1 * size_msg, position2 * size_msg, 0, (msg_nb - 1) * size_msg) == -1)
		{ return -1; };

	// Verifie la valeur des offsets (premiere et deuxieme case libre)
	char text2[] = "test_reception_type_neg() : ECHEC : offset case libre 1 dans la file apres reception 2eme msg.";
	if(offset_check(file, text2, size_msg * (position2 - position1), head->first_free) == -1) {return -1;}

	char text3[] = "test_reception_type_neg() : ECHEC : offset case libre 2 dans la file apres reception 2eme msg.";
	if(offset_check(file, text3, 0, head->last_free) == -1 ) {return -1;}

	return 0;
}


// Verifie les index et offsets apres receptions multiples
int test_receptions_multiples_fin(MESSAGE* file, int msg_nb, size_t size_msg,
		int position1, int position2, int position3){

	// Verifie les index une fois les appels a m_reception termines
	char text5[] = "test_receptions_multiples_fin() : ECHEC : indice apres reception de tous les messages.";
	if(index_check(file, text5, position1 * size_msg, (msg_nb - 1) * size_msg, 0, position3 * size_msg) == -1)
		{ return -1; };


	// Verifie la valeur des offsets des 3 premieres cases libres de la LC
	char text6[] = "test_receptions_multiples_fin() : ECHEC : offset 1ere case libre apres multiples receptions.";
	if(offset_check(file, text6, size_msg * (position2 - position1), position1 * size_msg) == -1) {return -1;}

	char text7[] = "test_receptions_multiples_fin() : ECHEC : offset 2eme case libre apres multiples receptions.";
	if(offset_check(file, text7, -2 * size_msg, position2 * size_msg) == -1) {return -1;}

	char text8[] = "test_receptions_multiples_fin() : ECHEC : offset 3eme case libre apres multiples receptions.";
	if(offset_check(file, text8, 3 * size_msg, 3 * size_msg) == -1) {return -1;}


	// Verifie la valeur des offsets des 3 cases occupees de la LC
	char text9[] = "test_receptions_multiples_fin() : ECHEC : offset 1ere case occupee apres multiples receptions.";
	if(offset_check(file, text9, size_msg, 0) == -1) {return -1;}

	char text10[] = "test_receptions_multiples_fin() : ECHEC : offset 2eme case occupee apres multiples receptions.";
	if(offset_check(file, text10, 3 * size_msg, size_msg) == -1) {return -1;}

	char text11[] = "test_receptions_multiples_fin() : ECHEC : offset 3eme/derniere case occupee apres multiples receptions.";
	if(offset_check(file, text11, 0, 4 * size_msg) == -1) {return -1;}

	return 0;
}


// Teste que les messages sont compactes en envoyant des messages plus petits que la taille maximum autorisee
int test_compact_messages(){
	int u[1] = {45};
	int small_msg_size = sizeof(struct mon_message) + sizeof(u);
	int big_msg_size = sizeof(struct mon_message) + sizeof(t);
	int big_msg_nb= 150;
	MESSAGE* file = m_connexion("/test_compact", O_RDWR | O_CREAT, big_msg_nb, sizeof(t), S_IRWXU | S_IRWXG | S_IRWXO);

	struct mon_message *m = malloc(sizeof(struct mon_message) + sizeof(u));
	if( m == NULL ){ perror("Function test malloc()"); exit(-1); }

	m->type = (long) getpid();
	memmove( m->mtext, u, sizeof(u));

	// Nombre de petits messages pouvant etre stockes dans la file
	int small_msg_nb = (int) floor(big_msg_size * big_msg_nb / small_msg_size);

	// Test envoi du nombre maximal de petits messages
	for(int j = 0; j < small_msg_nb; j++){
		if( m_envoi( file, m, sizeof(u), 0) != 0 ){
			printf("test_compact_messages() : ECHEC : envoie de %d petits message.\n", small_msg_nb); return -1;
		}

		// Verifie les valeurs des index tant que le tableau n'est pas plein
		size_t free_memory = file->memory_size - sizeof(header);

		if(small_msg_size * (j+2) <= free_memory){
			char text[] = "test_compact_messages() : ECHEC : indice envois multiples.";
			if(index_check(file, text, small_msg_size * (j+1), small_msg_size * (j+1), 0, small_msg_size * (j)) == -1)
				{ return -1; };
		}

		// Verifie les valeurs des index quand le tableau est plein
		else{
			char text[] = "test_compact_messages() : ECHEC : indice apres envoi dernier message.";
			if(index_check(file, text, -1, -1, 0, small_msg_size * (j)) == -1) { return -1; };
		}
	}

	// Verifie les offsets dans la file
	for(int j = 0; j < small_msg_nb; j++){
		if(j != small_msg_nb-1
				&& offset_check(file, "test_compact_messages() : ECHEC : offset.", small_msg_size, j*small_msg_size) == -1)
			{ return -1; }

		else if(j == small_msg_nb-1
				&& offset_check(file, "test_compact_messages() : ECHEC : offset dernier message.", 0, j*small_msg_size) == -1)
			{ return -1; }
	}

	// Test : envoi en mode non bloquant SANS place
	for(int j = 0; j < 2; j++){
		int i = m_envoi( file, m, sizeof(t), O_NONBLOCK);
		char text[] = "test_compact_messages() : ECHEC : gestion envoi dans un tableau plein (mode non bloquant).";

		if(error_check(text, i, EAGAIN) == -1) {return -1;}
	}
	// DEBUG : peut probablement factoriser toute les envois avec test_envois_multiples

	printf("test_compact_messages() : OK\n\n");
	return 0;
}

