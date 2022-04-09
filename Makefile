CPP=gcc # g++ ?
CFLAGS= -Wall -g -pedantic
LDLIBS = -pthread

all : functions-clem

functions-clem : functions-clem.o
	$(CPP) $(CFLAGS) $(LDLIBS) -o $@ $^


functions-clem.o : functions-clem.c
	$(CPP) $(CFLAGS) -c $<


clean :
	rm *.o

test: all
	./circuit
