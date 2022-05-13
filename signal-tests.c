#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

#include <signal.h>

int t[4] = {-12, 99, 134, 543};
// <>
void handler(int sig) {
	ssize_t s = m_reception(file, m1, sizeof(t), 0, 0);
}


int main(int argc, const char * argv[]) {
	// Connexion a une file (existante)
	MESSAGE* file = m_connexion("/envoi", O_RDWR);

	struct mon_message *m1 = malloc(sizeof(mon_message) + sizeof(t));
	if( m1 == NULL ){ perror("Function test malloc()"); exit(-1); }

	// Definit l'action a faire a la reception de SIGUSR1
	struct sigaction str;
	str.sa_handler = handler;
	sigfillset(&str.sa_mask);
	str.sa_flags = 0;

	if(sigaction(SIGUSR1, &str, NULL)<0){ perror("Function sigaction()"); exit(-1); }

	// Enregistrement du processus dans la file de messages
	long type = 1001;
	m_enregistrement(file, type, SIGUSR1);
}
