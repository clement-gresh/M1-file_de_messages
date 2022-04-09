CPP=gcc # g++ ?
CFLAGS= -Wall -g -pedantic
LDLIBS = -pthread



clem : functions-clem

functions-clem : functions-clem.o
	$(CPP) $(CFLAGS) $(LDLIBS) -o $@ $^


functions-clem.o : functions-clem.c
	$(CPP) $(CFLAGS) -c $<




hugo : functions-hugo

functions-hugo : functions-hugo.o
	$(CPP) $(CFLAGS) $(LDLIBS) -o $@ $^


functions-hugo.o : functions-hugo.c
	$(CPP) $(CFLAGS) -c $<




clean :
	rm *.o

test: all
	./circuit
