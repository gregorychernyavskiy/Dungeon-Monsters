CC = g++
CFLAGS = -Wall -Wextra -g -std=c++11
LDFLAGS = -lncurses
OBJ = main.o dungeon_generation.o save_load.o findpath.o minheap.o

all: dungeon

dungeon: $(OBJ)
	$(CC) $(CFLAGS) -o dungeon $(OBJ) $(LDFLAGS)

main.o: main.cpp dungeon_generation.h minheap.h
	$(CC) $(CFLAGS) -c main.cpp

dungeon_generation.o: dungeon_generation.cpp dungeon_generation.h minheap.h
	$(CC) $(CFLAGS) -c dungeon_generation.cpp

save_load.o: save_load.cpp dungeon_generation.h
	$(CC) $(CFLAGS) -c save_load.cpp

findpath.o: findpath.cpp dungeon_generation.h minheap.h
	$(CC) $(CFLAGS) -c findpath.cpp

minheap.o: minheap.cpp minheap.h
	$(CC) $(CFLAGS) -c minheap.cpp

clean:
	rm -f dungeon $(OBJ)

clobber:
	rm -f dungeon *.o