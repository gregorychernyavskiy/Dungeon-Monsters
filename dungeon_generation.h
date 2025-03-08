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

struct Stairs {
    int x;
    int y;
};

typedef struct {
    int x, y;           // Position
    int intelligent;    // 1 if intelligent, 0 otherwise
    int tunneling;      // 1 if tunneling, 0 otherwise
    int telepathic;     // 1 if telepathic, 0 otherwise
    int erratic;        // 1 if erratic, 0 otherwise
    int speed;          // Speed (5-20 for monsters, 10 for PC)
    int alive;          // 1 if alive, 0 if dead
    int last_seen_x;    // Last known PC x position (for intelligent non-telepathic)
    int last_seen_y;    // Last known PC y position
} Monster;

extern char dungeon[HEIGHT][WIDTH];         
extern unsigned char hardness[HEIGHT][WIDTH]; 
extern struct Room rooms[MAX_ROOMS];
extern int distance_non_tunnel[HEIGHT][WIDTH];
extern int distance_tunnel[HEIGHT][WIDTH];
extern Monster **monsters; // An array of pointers to Monster structs
extern int num_monsters;
extern Monster *monsterAt[HEIGHT][WIDTH];

extern int player_x;
extern int player_y;
extern int num_rooms; 
extern int randRoomNum;
extern int upStairsCount;
extern int downStairsCount;

extern char *dungeonFile;

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
void saveDungeon(char *filename);
void loadDungeon(char *filename);

void dijkstraNonTunneling(int dist[HEIGHT][WIDTH]);
void printNonTunnelingMap();
void dijkstraTunneling(int dist[HEIGHT][WIDTH]);
void printTunnelingMap();

Monster *createMonsterWithMonType(char c, int x, int y);
Monster *createMonster(int x, int y);
int spawnMonsterWithMonType(char monType);
int spawnMonsters(int count);  // Updated to return int
void moveMonster(Monster *monster);
void runGame(int numMonsters);

#endif