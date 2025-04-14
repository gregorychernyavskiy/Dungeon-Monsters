CC = g++
CFLAGS = -Wall -Wextra -g -std=c++11
LDFLAGS = -lncurses
OBJ = main.o dungeon_generation.o save_load.o findpath.o minheap.o monster_parsing.o object_parsing.o dice.o

all: dungeon

dungeon: $(OBJ)
	$(CC) $(CFLAGS) -o dungeon $(OBJ) $(LDFLAGS)

main.o: main.cpp dungeon_generation.h minheap.h monster_parsing.h object_parsing.h
	$(CC) $(CFLAGS) -c main.cpp

dungeon_generation.o: dungeon_generation.cpp dungeon_generation.h minheap.h
	$(CC) $(CFLAGS) -c dungeon_generation.cpp

save_load.o: save_load.cpp dungeon_generation.h
	$(CC) $(CFLAGS) -c save_load.cpp

findpath.o: findpath.cpp dungeon_generation.h minheap.h
	$(CC) $(CFLAGS) -c findpath.cpp

minheap.o: minheap.cpp minheap.h
	$(CC) $(CFLAGS) -c minheap.cpp

monster_parsing.o: monster_parsing.cpp monster_parsing.h dice.h
	$(CC) $(CFLAGS) -c monster_parsing.cpp

object_parsing.o: object_parsing.cpp object_parsing.h dice.h
	$(CC) $(CFLAGS) -c object_parsing.cpp

dice.o: dice.cpp dice.h
	$(CC) $(CFLAGS) -c dice.cpp

clean:
	rm -f dungeon $(OBJ)

clobber:
	rm -f dungeon *.o