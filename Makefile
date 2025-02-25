CC = gcc
CFLAGS = -Wall -Wextra -g
OBJ = main.o dungeon_generation.o save_load.o

all: dungeon

dungeon: $(OBJ)
	$(CC) $(CFLAGS) -o dungeon $(OBJ)

main.o: main.c dungeon_generation.h
	$(CC) $(CFLAGS) -c main.c

dungeon_generation.o: dungeon_generation.c dungeon_generation.h
	$(CC) $(CFLAGS) -c dungeon_generation.c

save_load.o: save_load.c dungeon_generation.h
	$(CC) $(CFLAGS) -c save_load.c

clean:
	rm -f dungeon $(OBJ)