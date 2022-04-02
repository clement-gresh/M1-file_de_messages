#ifndef M_FILE_H_
#define M_FILE_H_

typedef struct MESSAGE{
	enum{ RDWR,  RDONLY, WRONLY} flag;
	void* memory;
} MESSAGE;

#endif /* M_FILE_H_ */
