#include "project-tests.h"

// <>

int t[4] = {-12, 99, 134, 543};

// Faire la fonction tout_a_gauche_defragmentation

// Envoie puis recoit des messages petits pour verifier qu'on peut envoyer plus que nb_msg (memoire compacte)
// puis envoie des messages plus gros : verifier que la fonction fct_met_tout_a_gauche fonctionne

int main(int argc, const char * argv[]) {
	printf("\n");

	// Teste connexion et deconnexion
	test_connexion();
	test_deconnexion();
	test_destruction();

	// Teste l'envoi et la reception
	char name1[] = "/envoi1";
	MESSAGE* file = m_connexion(name1, O_RDWR | O_CREAT, 1, sizeof(t), S_IRWXU | S_IRWXG | S_IRWXO);
	test_envoi(file);
	test_reception(file);

	// Teste la bonne gestion des erreurs pour l'envoi et la reception
	test_envoi_erreurs();
	test_reception_erreurs();

	// Teste l'envoi et la reception de multiples messages
	char name2[] = "/tests_multiples1";
	int msg_nb = 20; // msg_nb doit etre superieur a 7 pour les tests
	MESSAGE* file2 = m_connexion(name2, O_RDWR | O_CREAT, msg_nb, sizeof(t), S_IRWXU | S_IRWXG | S_IRWXO);
	test_envois_multiples(file2, msg_nb);
	test_receptions_multiples(file2, msg_nb);
	test_compact_messages();

	// Destruction des files
	if(m_destruction(name1) == -1) { printf("main() : ECHEC : destruction 1\n"); return(-1); }
	if(m_destruction(name2) == -1) { printf("main() : ECHEC : destruction 2\n"); return(-1); }
	if(m_deconnexion(file) != 0){ perror("main : ECHEC : m_deconnexion(file) != 0\n"); return -1; }
	if(m_deconnexion(file2) != 0){ perror("main : ECHEC : m_deconnexion(file) != 0\n"); return -1; }

	// Teste la gestion de envoi et reception sur des processus en parallele
	for(int i = 0; i < 10; i++){ test_processus_paralleles(500); }
	printf("\n");
	/*
	// Teste l'enregistrement et l'envoi de signaux
	test_signal(0); // type = 0 (doit fonctionner)
	test_signal(998); // type positif (doit fonctionner)
	test_signal(-2000); // type negatif (doit fonctionner)
	// test_signal(3); // type positif (doit echouer, i.e. rester bloque)
	// test_signal(-10); // type negatif (doit echouer, i.e. rester bloque)
	*/
	return EXIT_SUCCESS;
}


// FONCTIONS DE VERIFICATION
// Verifie les valeurs du message recu (type, length, etc.)
int reception_check(mon_message *m1, char text[], ssize_t s, ssize_t size, size_t length, int value2, long type){
	if(s != size || m1->length != length || ((int*)m1->mtext)[2] != value2 || m1->type != type){
		printf("%s\n", text);
		printf("Valeurs attendues : retour %ld, length %ld, mtext %d, type %ld.\n", size, length, value2, type);
		printf("Valeurs recues : retour %ld, length %ld, mtext %d, type %ld.\n", s, m1->length, ((int*)m1->mtext)[2], m1->type);
		printf("\n\n");
		return -1;
	}
	return 0;
}

// Verifie les valeurs des index dans la file
int index_check(MESSAGE* file, char text[], int first_free, int last_free, int first_occupied, int last_occupied){
	header *head = &file->shared_memory->head;

	if(head->first_free != first_free || head->last_free != last_free
			|| head->first_occupied != first_occupied || head->last_occupied != last_occupied){

		printf("%s\n", text);
		printf("Indices cibles : %d, %d, %d, %d\n", first_free, last_free, first_occupied, last_occupied);
		printf("Indices reels : %d, %d, %d, %d\n",
				head->first_free, head->last_free, head->first_occupied, head->last_occupied);
		printf("\n\n");
		return -1;
	}
	return 0;
}

// Verifie que les 2 valeurs sont egales et renvoie une erreur sinon
int value_check(MESSAGE* file, char text[], long target_value, long actual_value){
	if( actual_value != target_value ){
		printf("%s\n", text);
		printf("Valeur voulue : %ld\n", target_value);
		printf("Valeur reelle : %ld\n", actual_value);
		printf("\n\n");
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
		printf("\n\n");
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
		printf("\n\n");
		return -1;
	}
	return 0;
}

// Verifie les differents attributs de la memoire partagee (taille memoire, longueur max d'un message,...)
int memory_check(MESSAGE* file, int msg_nb, size_t max_length){
	// Verifie le nombre minimum de messages que peut contenir la file
	if(value_check(file, "test_connexion_simple() : ECHEC pipe_capacity", msg_nb, m_capacite(file)) == -1)
		{ return -1; }

	// Verifie la taille maximum d'un message
	if(value_check(file, "test_connexion_simple() : ECHEC max_length", max_length,  m_message_len(file)) == -1)
		{ return -1; }

	// Verifie la memoire allouee
	size_t memory_size = sizeof(header) + (sizeof(mon_message) + max_length) * msg_nb;

	if(value_check(file, "test_connexion_simple() : ECHEC allocation memoire", memory_size,  file->memory_size) == -1)
		{ return -1; }

	// Verifie l'initialisation des index du tableau de message
	if(index_check(file, "test_connexion_simple() : ECHEC : erreur indices memoire.", 0, 0, -1, -1) == -1) { return -1; };

	return 0;
}

int envois_repetes(MESSAGE* file, int msg_nb, size_t msg_size, mon_message *m, int flag){
	size_t free_memory = file->memory_size - sizeof(header);

	for(int j = 0; j < msg_nb; j++){
		// Envoie des messages avec differentes valeurs de type
		m->type = abs((long) 1000 + pow(-2.0,(double) j));
		if( m_envoi( file, m, msg_size - sizeof(mon_message), flag) != 0 ){
			printf("ECHEC : envoie message n %d.", j + 1); return -1;
		}
		// Verifie le nombre de messages dans la file
		if(m_nb(file) != j + 1) { printf("ECHEC : nombre de messages dans la file."); return -1;}

		// Verifie les valeurs des index tant que le tableau n'est pas plein
		if(msg_size * (j+2) <= free_memory){
			char text[] = "ECHEC : indice envois repetes.";
			if(index_check(file, text, msg_size * (j+1), msg_size * (j+1), 0, msg_size * (j)) == -1) { return -1; };
		}
		// Verifie les valeurs des index quand le tableau est plein
		else{
			char text[] = "ECHEC : indice apres envoi dernier message.";
			if(index_check(file, text, -1, -1, 0, msg_size * (j)) == -1) { return -1; };
		}
	}
	// Verifie les offsets dans la file
	for(int j = 0; j < msg_nb; j++){
		if(j != msg_nb-1 && offset_check(file, " : ECHEC : offset.", msg_size, j*msg_size) == -1)
			{ return -1; }

		else if(j == msg_nb-1
				&& offset_check(file, "ECHEC : offset dernier message.", 0, j*msg_size) == -1)
			{ return -1; }
	}

	return 0;
}


// CONNEXION
// Teste les differents cas d'utilisation de m_connexion
int test_connexion(){
	char name1[] = "/nonono";
	if(test_connexion_simple(name1) == -1) { return -1; } //test connexion simple en O_RDWR | O_CREAT
	if(test_connexion_read_only() == -1) { return -1; } //test connexion simple en O_RDONLY | O_CREAT
	if(test_connexion_existe(name1) == -1) { return -1; } // test connexion sur une file existante
	if(test_connexion_anonyme() == -1) { return -1; } //tester la connexion a une file anonyme par un processus enfant
	if(test_connexion_excl() == -1) { return -1; } // tester la connexion en O_CREAT | O_EXCL

	printf("test_connexion() : OK\n\n");
	return 0;
}

// Teste la creation d'une file en O_RDWR | O_CREAT
int test_connexion_simple(char name[]){
	int msg_nb = 12;
	size_t max_length = sizeof(char)*20;

	// Teste creation de la file
	MESSAGE* file = m_connexion(name, O_RDWR | O_CREAT, msg_nb, max_length, S_IRWXU | S_IRWXG | S_IRWXO);
	if(file == NULL){ printf("test_connexion_simple() : ECHEC : NULL\n"); return -1; }

	// Verification des attributs de la file
	if( memory_check(file, msg_nb, max_length) == -1) { return -1; }

	return 0;
}

// Teste la creation d'une file avec O_RDONLY | O_CREAT (doit echouer)
int test_connexion_read_only(){
	char name[] = "/aaaaaaaaaaaaaaaa";
	int msg_nb = 12;
	size_t max_message_length = sizeof(char)*20;

	MESSAGE* file = m_connexion(name, O_RDONLY | O_CREAT, msg_nb, max_message_length, 0);
	if(file != NULL){ printf("test_connexion_read_only() : ECHEC : file != NULL\n"); return -1; }

	return 0;
}

// Teste la connexion a une file existante
int test_connexion_existe(char name[]){
	// Teste m_connexion sur une file existante (doit fonctionner)
	MESSAGE* file = m_connexion(name, O_RDWR);
	if(file == NULL){ printf("test_connexion_existe() : ECHEC : connexion a file existante\n"); return -1; }

	// Teste envoi d'un message sur la file (doit fonctionner)
	struct mon_message *m = malloc( sizeof( struct mon_message ) + sizeof( t ) );
	if( m == NULL ){ perror("Function test malloc()"); exit(-1); }
	m->type = (long) getpid();
	memmove( m->mtext, t, sizeof( t ));

	if( m_envoi( file, m, sizeof(t), 0) != 0 ){ printf("test_connexion_existe() : ECHEC : envoie message.\n"); return -1; }
	free(m);

	return 0;
}

// Teste la creation d'une file anonyme puis la connexion par un processus enfant
int test_connexion_anonyme(){
	int msg_nb = 12;
	size_t max_length = sizeof(char)*20;
	// Teste la creation de la file
	MESSAGE* file = m_connexion(NULL, O_RDWR | O_CREAT, msg_nb, max_length, S_IRWXU | S_IRWXG | S_IRWXO);
	if(file == NULL){ printf("test_connexion_anonyme() : ECHEC : file NULL\n"); return -1; }
	// Verification des attributs de la file
	if( memory_check(file, msg_nb, max_length) == -1) { return -1; }

	// Creation d'un fils
	pid_t pid = fork();
	if(pid == -1){ perror("test_connexion_anonyme fork() "); exit(-1); }

	else if(pid == 0){
		// Teste envoi d'un message sur la file par l'enfant
		struct mon_message *m = malloc( sizeof( struct mon_message ) + sizeof( t ) );
		if( m == NULL ){ perror("Function test malloc()"); exit(-1); }
		m->type = (long) getpid();
		memmove( m->mtext, t, sizeof( t ));

		if( m_envoi( file, m, sizeof(t), 0) != 0 ){ printf("test_connexion_anonyme() : ECHEC : envoie message.\n"); _exit(-1); }
		free(m);
		_exit(0);
	}
	else{
		// Le pere attend que l'enfant ait termine et verifie sa valeur de retour
		int status;
		if(wait(&status) == -1 && errno != ECHILD) { printf("test_connexion_anonyme() : ECHEC : wait ECHILD\n"); return(-1);}
		if(WIFEXITED(status) && WEXITSTATUS(status) != 0)
			{ printf("test_connexion_anonyme() : ECHEC : status child\n"); return(-1); }
	}
	return 0;
}

//  Teste la connexion en O_CREAT | O_EXCL
int test_connexion_excl(){
	char name[] = "/cecinestpasunnom2";
	int msg_nb = 12;
	size_t max_message_length = sizeof(char)*20;

	//printf("\n\n\navant connexion excl 1\n"); // debug
	// Test creation de la file avec O_CREAT | O_EXCL (doit fonctionner)
	MESSAGE* file = m_connexion(name, O_RDWR | O_CREAT | O_EXCL, msg_nb, max_message_length, S_IRWXU | S_IRWXG | S_IRWXO);
	if(file == NULL){ printf("test_connexion_excl() : ECHEC : creation file avec O_EXCL\n"); return -1; }

	//printf("apres connexion excl 1\n"); // debug
	// Test connexion a une file existante avec O_CREAT | O_EXCL (doit echouer)
	MESSAGE* file2 = m_connexion(name, O_RDWR | O_CREAT | O_EXCL, msg_nb, max_message_length, S_IRWXU | S_IRWXG | S_IRWXO);
	if(file2 != NULL){ printf("test_connexion_excl() : ECHEC : connexion file existante avec O_EXCL\n\n"); return -1; }

	if(m_destruction(name) != 0){ perror("test_connexion_excl : ECHEC : m_destruction()\n"); return -1; }
	if(m_deconnexion(file) != 0){ perror("test_connexion_excl : ECHEC : m_deconnexion(file) != 0\n"); return -1; }

	return 0;
}


// DECONNEXION
int test_deconnexion(){
	char name[] = "/NONO";
	int msg_nb = 12;
	size_t max_message_length = sizeof(char)*20;
	MESSAGE* file1 = m_connexion(name, O_RDWR | O_CREAT, msg_nb, max_message_length, S_IRWXU | S_IRWXG | S_IRWXO);
	if(file1 == NULL){ printf("test_deconnexion() : ECHEC : creation file1\n"); return -1; }

	MESSAGE* file2 = m_connexion(name, O_WRONLY);
	if(file2 == NULL){ printf("test_deconnexion() : ECHEC : creation file2\n"); return -1; }

	printf("AVANT connexion file3\n"); //debug
	MESSAGE* file3 = m_connexion(name, O_RDONLY);
	//MESSAGE* file3 = m_connexion(name, O_RDONLY | O_CREAT, msg_nb, max_message_length, S_IRWXU | S_IRWXG | S_IRWXO);
	//if(file3 == NULL){ printf("test_deconnexion() : ECHEC : creation file3\n"); return -1; }


	printf("file3 pipe capacity : %d\n", file3->shared_memory->head.pipe_capacity); //debug

	// Deconnexion sur file2
	printf("AVANT deconnexion file2\n"); //debug
	if(m_deconnexion(file2) != 0){ perror("test_deconnexion : ECHEC : m_deconnexion(file2) != 0\n"); return -1; }

	printf("AVANT test file2\n"); //debug
	// Teste que file2 n'est plus utilisable mais file1 et file3 le sont toujours
	if((void*)file2->shared_memory != NULL){
		perror("test_deconnexion : ECHEC : file2->shared_memory != NULL\n");
		return -1;
	}

	printf("AVANT test file1-3\n"); //debug
	if((void*)file1->shared_memory == NULL){
		perror("test_deconnexion : ECHEC : file1->shared_memory == NULL\n");
		return -1;
	}
	if((void*)file3->shared_memory == NULL){
		perror("test_deconnexion : ECHEC : file3->shared_memory == NULL\n");
		return -1;
	}

	printf("test_deconnexion() : OK\n\n");
	return 0;
}

int test_destruction(){
	char name[] = "/NONO";
	int msg_nb = 12;
	size_t max_message_length = sizeof(char)*20;
	MESSAGE* file1 = m_connexion(name, O_RDWR | O_CREAT, msg_nb, max_message_length, S_IRWXU | S_IRWXG | S_IRWXO);

	// Tentative de connexion avant destruction
	if(m_connexion(name, O_WRONLY) == NULL){
		perror("test_destruction : ECHEC : connexion impossible AVANT destruction\n");
		return -1;
	}

	if(fork()==0){ // enfant
		// Destruction de la file
		if(m_destruction(name) != 0){ perror("test_destruction : ECHEC : m_destruction retour != 0\n"); return -1; }

		// Tentative de connexion apres destruction (doit echouer)
		if(m_connexion(name, O_RDONLY) != NULL){
			perror("test_destruction : ECHEC : connexion possible apres destruction\n");
			_exit(-1);
		}
		_exit(0);
	}
	else{
		// Le pere attend que l'enfant ait termine et verifie sa valeur de retour
		int status;
		if(wait(&status) == -1 && errno != ECHILD) { printf("test_destruction() : ECHEC : wait ECHILD\n"); return(-1);}
		if(WIFEXITED(status) && WEXITSTATUS(status) != 0) { printf("test_destruction() : ECHEC : status child\n"); return(-1); }

		// Tentative de connexion apres destruction (doit echouer)
		if(m_connexion(name, O_WRONLY) != NULL){
			perror("test_destruction : ECHEC : connexion possible apres destruction\n");
			return -1;
		}

		// Test envoi d'un message sur file1 (doit toujours etre possible)
		struct mon_message *m = malloc( sizeof( struct mon_message ) + sizeof( t ) );
		if( m == NULL ){ perror("Function test malloc()"); exit(-1); }
		m->type = (long) getpid();
		memmove( m->mtext, t, sizeof( t ));

		if( m_envoi( file1, m, sizeof(t), 0) == -1 ){
			printf("test_destruction() : ECHEC de l'envoi d'un message apres destruction de la file.\n"); return -1;
		}
		if(m_deconnexion(file1) != 0){ perror("test_destruction : ECHEC : m_deconnexion(file) != 0\n"); return -1; }
		free(m);
	}

	printf("test_destruction() : OK\n\n");
	return 0;
}


// ENVOI
// Teste la bonne gestion des erreurs dans m_envoi
int test_envoi_erreurs(){
	struct mon_message *m = malloc( sizeof( struct mon_message ) + sizeof( t ) );
	if( m == NULL ){ perror("Function test malloc()"); exit(-1); }
	m->type = (long) getpid();
	memmove( m->mtext, t, sizeof( t ));

	// TESTE envoi d'un message trop long
	char name2[] = "/envoi_erreurs_2";
	size_t small_size = sizeof(t) - sizeof(char);
	MESSAGE* file2 = m_connexion(name2, O_RDWR | O_CREAT, 5, small_size, S_IRWXU | S_IRWXG | S_IRWXO);
	int i = m_envoi( file2, m, sizeof(t), O_NONBLOCK);
	if(error_check("test_envoi_erreurs() : ECHEC : gestion envoi d'un message trop long.", i, EMSGSIZE) == -1) {return -1;}
	if(m_destruction(name2) == -1) { printf("test_envoi_erreurs() : ECHEC : destruction 2\n"); return(-1); }
	if(m_deconnexion(file2) != 0){ perror("test_envoi_erreurs : ECHEC : m_deconnexion(file) != 0\n"); return -1; }

	// TESTE envoi avec mauvais drapeau
	char name3[] = "/envoi_erreurs_3";
	MESSAGE* file3 = m_connexion(name3, O_RDWR | O_CREAT, 5, sizeof(t), S_IRWXU | S_IRWXG | S_IRWXO);
	i = m_envoi( file3, m, sizeof(t), O_RDWR);
	if(error_check("test_envoi_erreurs() : ECHEC : gestion envoi avec mauvais drapeau.", i, EIO) == -1) {return -1;}

	// TESTE envoi dans une file en lecture seule
	MESSAGE* file1 = m_connexion(name3, O_RDONLY);
	i = m_envoi( file1, m, sizeof(t), O_NONBLOCK);
	char text[] = "test_envoi_erreurs() : ECHEC : gestion envoi dans un tableau read only.";
	if(error_check(text, i, EPERM) == -1) {return -1;}
	if(m_destruction(name3) == -1) { printf("test_envoi_erreurs() : ECHEC : destruction 3\n"); return(-1); }
	if(m_deconnexion(file3) != 0){ perror("test_envoi_erreurs : ECHEC : m_deconnexion(file) != 0\n"); return -1; }
	if(m_deconnexion(file1) != 0){ perror("test_envoi_erreurs : ECHEC : m_deconnexion(file) != 0\n"); return -1; }
	free(m);

	printf("test_envoi_erreurs() : OK\n\n");
	return 0;
}


// Teste m_envoi en envoyant un message quand la file est vide et un quand elle est pleine (mode non bloquant)
int test_envoi(MESSAGE* file){
	size_t msg_size = sizeof( struct mon_message ) + sizeof( t ) ;
	struct mon_message *m = malloc(msg_size);
	if( m == NULL ){ perror("Function test malloc()"); exit(-1); }
	memmove( m->mtext, t, sizeof( t ));

	// Test : envoi en mode non bloquant AVEC de la place
	if(envois_repetes(file, 1, msg_size, m, O_NONBLOCK) == -1)
		{ printf(" test_envoi()\n\n"); return -1; }

	// Test : envoi en mode non bloquant SANS place
	int i = m_envoi( file, m, sizeof(t), O_NONBLOCK);
	if(error_check("test_envoi() : ECHEC : gestion envoi dans un tableau plein (mode non bloquant).", i, EAGAIN) == -1)
		{return -1;}
	free(m);

	printf("test_envoi() : OK\n\n");
	return 0;
}


// Teste m_envoi en envoyant de multiples messages (file pleine et non pleine)
int test_envois_multiples(MESSAGE* file, int msg_nb){
	size_t msg_size = sizeof( struct mon_message ) + sizeof( t );
	struct mon_message *m = malloc(msg_size);
	if( m == NULL ){ perror("Function test malloc()"); exit(-1); }

	memmove( m->mtext, t, sizeof( t ));

	// Test : envois en mode non bloquant AVEC de la place
	if(envois_repetes(file, msg_nb, msg_size, m, O_NONBLOCK) == -1)
		{ printf(" test_envois_multiples()\n\n"); return -1; }

	// Test : envoi en mode non bloquant SANS place
	for(int j = 0; j < 2; j++){
		int i = m_envoi( file, m, sizeof(t), O_NONBLOCK);
		char text[] = "test_envois_multiples() : ECHEC : gestion envoi n dans un tableau plein (mode non bloquant).";

		if(error_check(text, i, EAGAIN) == -1) {return -1;}
	}
	free(m);

	printf("test_envois_multiples() : OK\n\n");
	return 0;
}


// RECEPTION
// Teste la bonne gestion des erreurs dans m_reception
int test_reception_erreurs(){
	struct mon_message *m1 = malloc( sizeof( struct mon_message ) + sizeof(t));
	if( m1 == NULL ){ perror("Function test malloc()"); exit(-1); }


	// Test reception avec drapeau incorrect
	char name[] = "/reception_erreurs_1";
	MESSAGE* file = m_connexion(name, O_RDWR | O_CREAT, 1, sizeof(t), S_IRWXU | S_IRWXG | S_IRWXO);
	if( m1 == NULL ){ perror("Function test malloc()"); exit(-1); }

	ssize_t s = m_reception(file, m1, sizeof(t), 0, O_RDWR); // O_RDWR au lieu de 0 ou O_NONBLOCK
	if(error_check("test_reception_erreurs() : ECHEC : gestion reception avec drapeau incorrect.", s, EIO) == -1)
		{return -1;}


	// Test reception dans un buffer trop petit (il faut d'abord envoyer un message dans la file)
	m1->type = (long) getpid();
	memmove( m1->mtext, t, sizeof(t));
	if( m_envoi( file, m1, sizeof(t), 0) != 0 ){ printf("test_reception_erreurs() : ECHEC : envoi.\n"); return -1; }

	s = m_reception(file, m1, 0, 0, O_NONBLOCK); // 0 au lieu de sizeof(t)
	if(error_check("test_reception_erreurs() : ECHEC : gestion buffer trop petit.", s, EMSGSIZE) == -1)
		{return -1;}

	// Destruction de la file
	if(m_destruction(name) == -1) { printf("test_reception_erreurs() : ECHEC : destruction\n"); return(-1); }
	if(m_deconnexion(file) != 0){ perror("test_reception_erreurs : ECHEC : m_deconnexion(file) != 0\n"); return -1; }
	free(m1);

	printf("test_reception_erreurs() : OK\n\n");
	return 0;
}


// Teste m_reception en tentant de recuperer un message de la file quand elle est pleine ou vide (mode non bloquant)
int test_reception(MESSAGE* file){
	struct mon_message *m1 = malloc(sizeof(mon_message) + sizeof(t));
	if( m1 == NULL ){ perror("Function test malloc()"); exit(-1); }

	// Reception quand il y a bien un message dans la file
	ssize_t s = m_reception(file, m1, sizeof(t), 0, 0);

	if(reception_check(m1, "test_reception() : ECHEC : reception.", s, sizeof(t), sizeof(t), t[2], 1001) == -1)
		{return -1;}

	// Verifie les valeurs des index
	if(index_check(file, "test_reception() : ECHEC : indices apres reception 1er message.", 0, 0, -1, -1) == -1) {return -1;};

	// Verifie la valeur de l'offset dans la file
	if(offset_check(file, "test_reception() : ECHEC : offset case vide.", 0, 0) == -1) {return -1;}

	// Reception en mode NON bloquant alors qu'il n'y a pas de message dans la file
	s = m_reception(file, m1, sizeof(t), 0, O_NONBLOCK);
	if(error_check("test_reception() : ECHEC : gestion reception d'un tableau vide (mode non bloquant).", s, EAGAIN) == -1)
		{return -1;}
	free(m1);

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

	// Teste la reception de multiples messages en mode non bloquant avec differentes valeurs de 'type'
	if(test_receptions_multiples_milieu(file, m1, size_msg, msg_nb, position1, position2, position3) == -1)
		{return -1;}

	// Verifie les index et offsets apres receptions multiples
	if(test_receptions_multiples_fin(file, msg_nb, size_msg, position1, position2, position3) == -1)
		{return -1;}
	free(m1);

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
	if(reception_check(m1, text0, s, size_msg - sizeof(mon_message), size_msg - sizeof(mon_message), t[2], 1004) == -1)
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
	if(reception_check(m1, text0, s, size_msg - sizeof(mon_message), size_msg - sizeof(mon_message), t[2], 968) == -1)
		{return -1;}

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

// Teste la reception de multiples messages en mode non bloquant avec differentes valeurs de 'type'
int test_receptions_multiples_milieu(MESSAGE* file, mon_message *m1, size_t size_msg, int msg_nb,
		int position1, int position2, int position3){

	for(int j = 2; j < msg_nb; j++){
		long type;
		if(j == 4){ type = -2; }
		else{ type = abs((long) 1000 + pow(-2.0,(double) j)); }

		ssize_t s = m_reception(file, m1, sizeof(t), type, O_NONBLOCK);

		// Reception quand il n'y a pas de message de type demande (positif ou negatif), mode non bloquant
		if(j == position1 || j == position2 || j == position3){
			char text[] = "test_reception_multiples_milieu() : ECHEC : gestion absence message type (non bloquant).";
			if(error_check(text, s, EAGAIN) == -1) {return -1;}
		}

		// Reception des autres messages
		else{
			// Verifie les donnees des messages
			char text0[] = "test_reception_multiples_milieu() : ECHEC : reception multiples.";
			if(reception_check(m1, text0, s, sizeof(t), sizeof(t), t[2], type) == -1) {return -1;}

			// Verifie les offsets (case lue et case precedente)
			char text1[] = "test_reception_multiples_milieu() : ECHEC : offset derniere case lue.";
			if(j > 6 && offset_check(file, text1, 0, size_msg * j) == -1) {return -1;}

			char text2[] = "test_reception_multiples_milieu() : ECHEC : offset precedente case lue.";
			if(j > 6 && offset_check(file, text2, size_msg, size_msg * (j-1)) == -1) {return -1;}
		}
	}
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


// MESSAGES COMPACTES
// Teste que les messages sont compactes en envoyant des messages plus petits que la taille maximum autorisee
int test_compact_messages(){
	int u[1] = {45};
	int small_msg_size = sizeof(struct mon_message) + sizeof(u);
	int big_msg_size = sizeof(struct mon_message) + sizeof(t);
	int big_msg_nb= 150;
	char name[] = "/test_compact";
	MESSAGE* file = m_connexion(name, O_RDWR | O_CREAT, big_msg_nb, sizeof(t), S_IRWXU | S_IRWXG | S_IRWXO);

	struct mon_message *small_m = malloc(sizeof(struct mon_message) + sizeof(u));
	if( small_m == NULL ){ perror("Function test malloc()"); exit(-1); }
	memmove( small_m->mtext, u, sizeof(u));

	// Nombre de petits messages pouvant etre stockes dans la file
	int small_msg_nb = (int) floor(big_msg_size * big_msg_nb / small_msg_size);

	// Test envoi du nombre maximal de petits messages (doit reussir)
	if(envois_repetes(file, small_msg_nb, small_msg_size, small_m, O_NONBLOCK) == -1)
		{ printf(" test_compact_messages()\n\n"); return -1; }


	// Test : envoi en mode non bloquant SANS place (doit echouer)
	for(int j = 0; j < 2; j++){
		int i = m_envoi( file, small_m, sizeof(u), O_NONBLOCK);
		char text[] = "test_compact_messages() : ECHEC : gestion envoi dans un tableau plein (mode non bloquant).";

		if(error_check(text, i, EAGAIN) == -1) {return -1;}
	}

	// Test : defragmentation
	test_defragmentation(file, small_m, small_msg_nb, small_msg_size);

	// Destruction de la file
	if(m_destruction(name) == -1) { printf("test_compact_messages() : ECHEC : destruction\n"); return(-1); }
	if(m_deconnexion(file) != 0){ perror("test_compact_messages : ECHEC : m_deconnexion(file) != 0\n"); return -1; }
	free(small_m);

	printf("test_compact_messages() : OK\n\n");
	return 0;
}

// Fait m_reception sur 2 petits messages non consecutifs puis essaye d'envoyer un gros message (en mode bloquant)
int test_defragmentation(MESSAGE* file, mon_message* small_m, int small_msg_nb, size_t small_msg_size){
	int position1 = 2; // position of message with type 1004
	int position2 = 5; // position of message with type 968

	// Reception de 2 petits messages
	if(test_reception_type_pos(file, small_m, small_msg_size, small_msg_nb, position1) == -1) {return -1;}
	if(test_reception_type_neg(file, small_m, small_msg_size, small_msg_nb, position1, position2) == -1) {return -1;}

	struct mon_message *big_m = malloc(sizeof(struct mon_message) + sizeof(t));
	if( big_m == NULL ){ perror("Function test malloc()"); exit(-1); }
	memmove( big_m->mtext, t, sizeof(t));

	// Envoi d'un gros message
	if(m_envoi( file, big_m, sizeof(t), 0) != 0)
		{ printf("test_defragmentation() : ECHEC : envoie gros message.\n"); return -1; }

	// Verifie le nombre de messages dans la file
	if(m_nb(file) != small_msg_nb - 1)
		{ printf("test_defragmentation() : ECHEC : nombre de messages dans la file (gros message).\n"); return -1;}


	// Verifie les valeurs des index tant que le tableau n'est pas plein
	char text[] ="test_defragmentation() : ECHEC : indice apres envoi gros message.";
	if(index_check(file, text, -1, -1, 0, small_msg_size * (small_msg_nb - 2)) == -1)
		{ return -1; };

	// Verifie les offsets dans la file
	for(int j = 0; j < small_msg_nb - 1; j++){
		if(j != small_msg_nb-2
				&& offset_check(file, "test_defragmentation() : ECHEC : offset.", small_msg_size, j*small_msg_size) == -1)
			{ return -1; }

		else if(j == small_msg_nb-2
				&& offset_check(file, "test_defragmentation ECHEC : offset dernier message.", 0, j*small_msg_size) == -1)
			{ return -1; }
	}
	free(big_m);

	printf("test_defragmentation() : OK\n\n");
	return 0;
}


// PARALLELISME
// Test l'envoi et la reception par des processus en parallele
// 4 processus en parallele : 2 envoient le nombre maximum de message et 2 en receptionne le nombre maximum (en mode bloquant)
int test_processus_paralleles(int msg_nb){
	size_t size_msg = sizeof( struct mon_message ) + sizeof( t );

	// message a envoyer
	struct mon_message *me = malloc(size_msg);
	if( me == NULL ){ perror("Function test malloc()"); exit(-1); }
	memmove( me->mtext, t, sizeof( t ));

	// structure pour recevoir un message
	struct mon_message *mr = malloc(size_msg);

	// Creation de la file
	MESSAGE* file = m_connexion(NULL, O_RDWR | O_CREAT, msg_nb, sizeof(t), S_IRWXU | S_IRWXG | S_IRWXO);

	// Le pere cree un fils A
	pid_t pidA = fork();
	if(pidA == -1){ perror("test_processus_paralleles fork() A"); exit(-1); }

	// Le pere et A creent tous les 2 un fils B
	pid_t pidB = fork();
	if(pidB == -1){ perror("test_processus_paralleles fork() B"); exit(-1); }

	// Les 2 fils B : chacun RECEPTIONNE (en mode bloquant) le nombre maximal de messages que peut contenir la file
	if(pidB == 0){
		for(int j = 0; j < msg_nb; j++){
			if( m_reception(file, mr, sizeof(t), 0, 0) == 0 ){
				printf("test_processus_paralleles() : ECHEC : receptions messages n %d.\n", j + 1); _exit(-1);
			}
		}
		_exit(0);
	}
	// Pere et fils A : chacun ENVOIE (en mode bloquant) le nombre maximal de messages que peut contenir la file
	else if(pidB > 0){
		for(int j = 0; j < msg_nb; j++){
			if( m_envoi( file, me, sizeof(t), 0) != 0 ){
				printf("test_processus_paralleles() : ECHEC : envois messages n %d.\n", j + 1);
				if(pidA == 0){ _exit(-1); }
				return -1;
			}
		}
		// Le fils A
		if(pidA == 0){ _exit(0); }
	}
	// Le pere attend que les enfants aient termine et verifie leur valeur de retour
	int status;
	while(wait(&status) > 0)
		if(status == -1) { printf("test_processus_paralleles() : ECHEC : status child\n"); return(-1); }
	if(WIFEXITED(status) && WEXITSTATUS(status) != 0) { printf("test_processus_paralleles() : ECHEC : status child\n"); return(-1); }
	if(errno != ECHILD) { printf("test_processus_paralleles() : ECHEC : wait ECHILD\n"); return(-1);}

	// Destruction de la file
	if(m_deconnexion(file) != 0){ perror("test_processus_paralleles : ECHEC : m_deconnexion(file) != 0\n"); return -1; }
	free(me);
	free(mr);

	printf("test_processus_paralleles() : OK\n");
	return 0;
}


// Teste l'envoi d'un signal a un processus s'etant enregistre sur la file via m_enregistrement
void handler1(int sig) {}

int test_signal(long type){
	// Creation de la file
	char name1[] = "/test_signal_27";
	size_t msg_size = sizeof( struct mon_message ) + sizeof( t ) ;
	MESSAGE* file = m_connexion(name1, O_RDWR | O_CREAT , 10, sizeof(t), S_IRWXU | S_IRWXG | S_IRWXO);

	// Definit l'action a faire a la reception de SIGUSR1
	struct sigaction str;
	str.sa_handler = handler1;
	sigfillset(&str.sa_mask);
	str.sa_flags = 0;
	if(sigaction(SIGUSR1, &str, NULL)<0){ perror("Function sigaction()"); exit(-1); }

	// Applique un masque contenant seulement SIGUSR1
	sigset_t block_set, previous_set;
	sigemptyset(&block_set);
	sigaddset(&block_set, SIGUSR1);
	if(sigprocmask(SIG_SETMASK, &block_set, &previous_set) < 0){ perror("Function sigprocmask()"); exit(-1); }

	// Enregistre le processus sur la file d'attente de la file
	if(m_enregistrement(file, type, SIGUSR1) == -1) { printf("test_signal() : ECHEC : m_enregistrement.\n"); return(-1); }

	// Creation d'un fils A
	pid_t pidA = fork();
	if(pidA == -1){ perror("test_signal fork() A"); exit(-1); }

	else if(pidA == 0){
		// Creation d'un petit fils B
		pid_t pidB = fork();
		if(pidB == -1){ perror("test_signal fork() B"); _exit(-1); }

		else if(pidB == 0){
			// Le petit-fils B demande a recevoir un message de type 0 en mode bloquant
			// Il doit recevoir le 1er envoye par A, donc celui de type 1001
			struct mon_message *mb = malloc(msg_size);
			if( mb == NULL ){ perror("Function test malloc()"); _exit(-1); }
			size_t s = m_reception(file, mb, sizeof(t), 0, 0);
			if(reception_check(mb, "test_signal() : ECHEC : reception.", s, sizeof(t), sizeof(t), t[2], 1001) == -1)
				{ _exit(-1); }
			free(mb);
			_exit(0);
		}
		else{
			// Le fils A envoie 2 messages dans la file
			sleep(1); // sleep pour assurer que B est deja en attente sur la file

			struct mon_message *ma = malloc(msg_size);
			if( ma == NULL ){ perror("Function test malloc()"); exit(-1); }
			memmove( ma->mtext, t, sizeof( t ));

			if(envois_repetes(file, 2, msg_size, ma, O_NONBLOCK) == -1)
				{ printf(" test_signal()\n\n");  _exit(-1); }
			free(ma);

			// A attend que l'enfant B ait termine et verifie sa valeur de retour
			int status;
			if(wait(&status) == -1 && errno != ECHILD) { printf("test_signal() : ECHEC : wait ECHILD\n"); _exit(-1);}
			if(WIFEXITED(status) && WEXITSTATUS(status) != 0) { printf("test_signal() : ECHEC : status child\n"); _exit(-1); }
			_exit(0);
		}
	}
	else{
		// Le pere attend de recevoir SIGUSR1 pour faire m_reception
		sigsuspend(&previous_set);

		// Une fois le signal recu, le pere appelle m_reception et doit recuperer le 2eme message envoye par A (type 998)
		struct mon_message *m = malloc(msg_size);
		if( m == NULL ){ perror("Function test malloc()"); exit(-1); }
		size_t s = m_reception(file, m, sizeof(t), 0, 0);
		if(reception_check(m, "test_signal() : ECHEC : reception.", s, sizeof(t), sizeof(t), t[2], 998) == -1)
			{ return(-1); }
		free(m);

		// Remet le mask a sa valeur initiale
		if(sigprocmask(SIG_SETMASK, &previous_set, NULL) < 0){ perror("Function sigprocmask()"); exit(-1); }

		// Le pere attend que l'enfant A ait termine et verifie sa valeur de retour
		int status;
		if(wait(&status) == -1 && errno != ECHILD) { printf("test_signal() : ECHEC : wait ECHILD\n"); return(-1);}
		if(WIFEXITED(status) && WEXITSTATUS(status) != 0) { printf("test_signal() : ECHEC : status child\n"); return(-1); }
	}

	// Destruction de la file
	if(m_destruction(name1) == -1) { printf("test_signal() : ECHEC : destruction\n"); return(-1); }
	if(m_deconnexion(file) != 0){ perror("test_signal : ECHEC : m_deconnexion(file) != 0\n"); return -1; }

	printf("test_signal() : OK\n\n");
	return 0;
}

