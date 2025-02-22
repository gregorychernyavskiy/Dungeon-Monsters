#ifndef DUNGEON_GENERATION_H
#define DUNGEON_GENERATION_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <time.h>
#include <endian.h>

#define WIDTH 80
#define HEIGHT 21

#define MAX_ROOMS 8
#define MIN_ROOMS 6

#define MIN_ROOM_HEIGHT 3
#define MAX_ROOM_HEIGHT 9

#define MIN_ROOM_WIDTH 4
#define MAX_ROOM_WIDTH 12

#define MAX_HARDNESS 255
#define MIN_HARDNESS 1

struct Room {
    int x;
    int y;
    int height;
    int width;
};

struct Stairs {
    int x;
    int y;
};

extern char dungeon[HEIGHT][WIDTH];         
extern unsigned char hardness[HEIGHT][WIDTH]; 
extern struct Room rooms[MAX_ROOMS];

extern int player_x;
extern int player_y;
extern int num_rooms; 
extern int randRoomNum;
extern int upStairsCount;
extern int downStairsCount;

extern struct Stairs upStairs[MAX_ROOMS];
extern struct Stairs downStairs[MAX_ROOMS];

void printDungeon();
void emptyDungeon();
int overlapCheck(struct Room r1, struct Room r2);
void createRooms(void);
void connectRooms(void);
void placeStairs(void);
void placePlayer(void);
void initializeHardness();
void printHardness();
void saveDungeon();
void loadDungeon();

#endif