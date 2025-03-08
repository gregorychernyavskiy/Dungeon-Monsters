#ifndef DUNGEON_GENERATION_H
#define DUNGEON_GENERATION_H


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <time.h>
#include <endian.h>
#include <limits.h>
#include "minheap.h"


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


typedef struct Tile {
    char type;
    int hardness;
    int tunnelingDist;
    int nonTunnelingDist;
} Tile;


struct Stairs {
    int x; 
    int y;
};


typedef struct Monster {
    int intelligent;
    int tunneling;
    int telepathic;
    int erratic;
    int speed;
    Pos pos;
    Pos lastSeen;
} Mon;



extern char dungeon[HEIGHT][WIDTH];
extern unsigned char hardness[HEIGHT][WIDTH];
extern struct Room rooms[MAX_ROOMS];
extern int distance_non_tunnel[HEIGHT][WIDTH];
extern int distance_tunnel[HEIGHT][WIDTH];
extern int player_x, player_y, num_rooms, randRoomNum, upStairsCount, downStairsCount;
extern struct Stairs upStairs[MAX_ROOMS], downStairs[MAX_ROOMS];
extern Monster* monsters[HEIGHT * WIDTH]; // Larger array for monsters
extern int num_monsters;



void printDungeon();
void emptyDungeon();
int overlapCheck(struct Room r1, struct Room r2);
void createRooms();
void connectRooms();
void placeStairs();
void placePlayer();
void initializeHardness();
void printHardness();
void saveDungeon(char *filename);
void loadDungeon(char *filename);
void dijkstraNonTunneling(int dist[HEIGHT][WIDTH]);
void printNonTunnelingMap();
void dijkstraTunneling(int dist[HEIGHT][WIDTH]);
void printTunnelingMap();

Monster* createMonster(int x, int y); // General monster creation
int spawnMonsters(int numMonsters);
int populateDungeon(int numMonsters);
int fillDungeon(int numMonsters);
void cleanup();

#endif