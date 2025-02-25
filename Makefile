CC = gcc
CFLAGS = -Wall -Werror -ggdb -funroll-loops
LDFLAGS = 

OBJS = main.o dungeon_generation.o save_load.o pathfinding.o

all: rlg327

rlg327: $(OBJS)
	$(CC) $(CFLAGS) $(OBJS) -o rlg327 $(LDFLAGS)

main.o: main.c dungeon_generation.h
	$(CC) $(CFLAGS) -c main.c

dungeon_generation.o: dungeon_generation.c dungeon_generation.h
	$(CC) $(CFLAGS) -c dungeon_generation.c

save_load.o: save_load.c dungeon_generation.h
	$(CC) $(CFLAGS) -c save_load.c

pathfinding.o: pathfinding.c dungeon_generation.h priority_queue.h
	$(CC) $(CFLAGS) -c pathfinding.c

clean:
	rm -f *.o rlg327 *~

.PHONY: clean