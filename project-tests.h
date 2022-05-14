#ifndef PROJECT_TESTS_H_
#define PROJECT_TESTS_H_

#include <math.h>

#include "m_file.h"

// FONCTIONS DE TEST
// Verifications
int reception_check(mon_message *m1, char text[], ssize_t s, ssize_t size, size_t length, int value2, long type);
int index_check(MESSAGE* file, char text[], int first_free, int last_free, int first_occupied, int last_occupied);
int offset_check(MESSAGE* file, char text[], int offset, int position);
int error_check(char text[], ssize_t s, int error);

// Connexion
int test_connexion();
int test_connexion_simple(char name[]);
int test_connexion_read_only();
int test_connexion_existe(char name[]);
int test_connexion_anonyme();
int test_connexion_excl();

// Deconnexion
int test_deconnexion();
int test_destruction();

// Envoi
int test_envoi_erreurs();
int test_envoi(MESSAGE* file);
int test_envois_multiples(MESSAGE* file, int msg_nb);

// Reception
int test_reception_erreurs();
int test_reception(MESSAGE* file);
int test_receptions_multiples(MESSAGE* file, int msg_nb);
int test_reception_type_pos(MESSAGE* file, mon_message *m1, size_t size_msg, int msg_nb, int position1);
int test_reception_type_neg(MESSAGE* file, mon_message *m1, size_t size_msg, int msg_nb, int position1, int position2);
int test_receptions_multiples_milieu(MESSAGE* file, mon_message *m1, size_t size_msg, int msg_nb,
		int position1, int position2, int position3);
int test_receptions_multiples_fin(MESSAGE* file, int msg_nb, size_t size_msg,
		int position1, int position2, int position3);

// Messages compactes
int test_compact_messages();

// Parallelisme
int test_processus_paralleles(int msg_nb);

#endif /* PROJECT_TESTS_H_ */
