#include "m_file.h"


int main(int argc, const char * argv[]) {

	MESSAGE* file = m_connexion("/kangourou", O_RDWR | O_CREAT, 12, sizeof(char)*50, S_IRWXU | S_IRWXG | S_IRWXO);

	return EXIT_SUCCESS;
}
