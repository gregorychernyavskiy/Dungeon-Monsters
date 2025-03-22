CC = gcc
CFLAGS = -Wall -Wextra -g
LDFLAGS = -lncurses
OBJ = main.o dungeon_generation.o save_load.o findpath.o minheap.o

all: dungeon

dungeon: $(OBJ)
	$(CC) $(CFLAGS) -o dungeon $(OBJ) $(LDFLAGS)

main.o: main.c dungeon_generation.h minheap.h
	$(CC) $(CFLAGS) -c main.c

dungeon_generation.o: dungeon_generation.c dungeon_generation.h minheap.h
	$(CC) $(CFLAGS) -c dungeon_generation.c

save_load.o: save_load.c dungeon_generation.h
	$(CC) $(CFLAGS) -c save_load.c

findpath.o: findpath.c dungeon_generation.h minheap.h
	$(CC) $(CFLAGS) -c findpath.c

minheap.o: minheap.c minheap.h
	$(CC) $(CFLAGS) -c minheap.c

clean:
	rm -f dungeon $(OBJ)