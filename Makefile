CC = gcc
CFLAGS = -Wall -Wextra -g
LDFLAGS = -lncurses  # Added for ncurses library linking
OBJ = main.o dungeon_generation.o minheap.o

all: dungeon

dungeon: $(OBJ)
	$(CC) $(CFLAGS) -o dungeon $(OBJ) $(LDFLAGS)

main.o: main.c dungeon_generation.h minheap.h
	$(CC) $(CFLAGS) -c main.c

dungeon_generation.o: dungeon_generation.c dungeon_generation.h minheap.h
	$(CC) $(CFLAGS) -c dungeon_generation.c

minheap.o: minheap.c minheap.h
	$(CC) $(CFLAGS) -c minheap.c

clean:
	rm -f dungeon $(OBJ)