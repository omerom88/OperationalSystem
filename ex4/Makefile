
all: CachingFileSystem

CachingFileSystem: Block.o Cache.o CachingFileSystem.o
	g++ -Wall -std=c++11 -D_FILE_OFFSET_BITS=64 CachingFileSystem.o Cache.o Block.cpp `pkg-config fuse --cflags --libs` -o CachingFileSystem
CachingFileSystem.o: Block.h CachingFileSystem.cpp Cache.h
	g++ -Wall -std=c++11 -D_FILE_OFFSET_BITS=64 -c CachingFileSystem.cpp
Cache.o: Block.h Cache.cpp Cache.h
	g++ -Wall -Wextra -Wvla -std=c++11 -D_FILE_OFFSET_BITS=64 -c Cache.cpp

Block.o: Block.h Block.cpp
	g++ -Wall -Wextra -Wvla -std=c++11 -D_FILE_OFFSET_BITS=64 -c Block.cpp
tar:
	tar -cvf ex4.tar Block.cpp Block.h Cache.cpp Cache.h CachingFileSystem.cpp README Makefile
clean:
	rm -f Block.o CachingSystem.o Block.o

.PHONY: CachingFileSystem CachingFileSystem.o clean tar
