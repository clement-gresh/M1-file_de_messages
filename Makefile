CPP=gcc
CFLAGS= -Wall -std=gnu11 -g -pedantic 
LDLIBS = -pthread
LIBS = -lrt -lm


all : project-tests

project-tests : project-tests.o m_file.o
	$(CPP) $(CFLAGS) $(LDLIBS) -o $@ $^ $(LIBS)

project-tests.o : project-tests.c
	$(CPP) $(CFLAGS) -c $<

m_file.o : m_file.c
	$(CPP) $(CFLAGS) -c $<

clean :
	rm *.o

test: all
	./project-tests

launch : clean all
	./project-tests