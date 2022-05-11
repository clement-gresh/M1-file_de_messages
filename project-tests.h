#ifndef PROJECT_TESTS_H_
#define PROJECT_TESTS_H_

#include <math.h>

#include "m_file.h"

// FONCTIONS DE TEST
int reception_check(mon_message *m1, char text[], ssize_t s, ssize_t size, size_t length, int value2, long type);
int index_check(MESSAGE* file, char text[], int first_free, int last_free, int first_occupied, int last_occupied);
int offset_check(MESSAGE* file, char text[], int offset, int position);
int error_check(char text[], ssize_t s, int error);
int test_connexion();
int test_envoi_erreurs();
int test_envoi(MESSAGE* file);
int test_envois_multiples(MESSAGE* file, int msg_nb);
int test_reception_erreurs();
int test_reception(MESSAGE* file);
int test_receptions_multiples(MESSAGE* file, int msg_nb);
int test_reception_type_pos(MESSAGE* file, mon_message *m1, size_t size_msg, int msg_nb, int position1);
int test_reception_type_neg(MESSAGE* file, mon_message *m1, size_t size_msg, int msg_nb, int position1, int position2);
int test_receptions_multiples_fin(MESSAGE* file, int msg_nb, size_t size_msg,
		int position1, int position2, int position3);
int test_compact_messages();

#endif /* PROJECT_TESTS_H_ */
