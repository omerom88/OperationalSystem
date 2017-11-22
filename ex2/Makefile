INCS=-I.
CFLAGS = -Wall -g -c -std=c++11 $(INCS)
TARGETS = Makefile README uthreads.cpp uthreads.h ReadyQueue.cpp ReadyQueue.h Thread.cpp Thread.h

all: libuthreads.a

libuthreads.a :  ReadyQueue.o Thread.o uthreads.o
	ar rvs libuthreads.a ReadyQueue.o Thread.o uthreads.o

uthreads.o :  uthreads.cpp uthreads.h ReadyQueue.o Thread.o
	g++ $(CFLAGS) uthreads.cpp

ReadyQueue.o :  ReadyQueue.cpp ReadyQueue.h Thread.o
	g++ $(CFLAGS) ReadyQueue.cpp

Thread.o :  Thread.cpp Thread.h
	g++ $(CFLAGS) Thread.cpp 

.PHONY : clean

tar : $(TARGETS)
	tar cvf ex2.tar $(TARGETS)

clean:
	rm *.o
	rm libuthreads.a
	
