CPP=gcc # g++ ?
CFLAGS= -Wall -g -pedantic

all : functions-clem

functions-clem : functions-clem.o
	$(CPP) $(CFLAGS) -o $@ $^


functions-clem.o : functions-clem.c
	$(CPP) $(CFLAGS) -c $<


clean :
	rm *.o

test: all
	./circuit
