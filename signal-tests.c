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

/*

	// DEBUG
	//printf("First free : %d. First occupied : %d\n", head->first_free, head->first_occupied);
	printf("Debut m_envoi.\nLC LIBRES\n");
	int temp = head->first_free;
	int i = 1;
	if(temp == -1) { printf("vide\n"); }
	else{
		while(true){
			printf("Case %d : postion %d, offset %ld, length %ld\n",
					i, temp, ((mon_message *)&messages[temp])->offset, ((mon_message *)&messages[temp])->length);
			if(((mon_message *)&messages[temp])->offset == 0){
				break;
			}
			temp = temp + ((mon_message *)&messages[temp])->offset;
			i++;
		}
	}

	printf("LC OCCUPEES\n");
	int temp2 = head->first_occupied;
	int j = 1;
	if(temp2 == -1) { printf("vide\n"); }
	else{
		while(true){
			printf("Case %d : postion %d, offset %ld, length %ld\n",
					j, temp2, ((mon_message *)&messages[temp2])->offset, ((mon_message *)&messages[temp2])->length);
			if(((mon_message *)&messages[temp2])->offset == 0){
				break;
			}
			temp2 = temp2 + ((mon_message *)&messages[temp2])->offset;
			j++;
		}
	}
	printf("\n");
	//FIN DEBUG

 */
