CPP=gcc # g++ ?
CFLAGS= -Wall -std=gnu11 -g -pedantic 
LDLIBS = -pthread
LIBS = -lrt -lm


all : project-tests

project-tests : project-tests.o functions-clem.o  functions-hugo.c
	$(CPP) $(CFLAGS) $(LDLIBS) -o $@ $^ $(LIBS)

project-tests.o : project-tests.c
	$(CPP) $(CFLAGS) -c $<


functions-clem.o : functions-clem.c
	$(CPP) $(CFLAGS) -c $<


functions-hugo.o : functions-hugo.c
	$(CPP) $(CFLAGS) -c $<
	


clean :
	rm *.o

test: all
	./project-tests

launch : clean all
	./project-tests