#ifndef DUNGEON_GENERATION_H
#define DUNGEON_GENERATION_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <time.h>
#include <endian.h>
#include <limits.h>

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
#define INFINITY INT_MAX
#define DEFAULT_MONSTERS 10
#define PC_SPEED 10
#define MIN_MONSTER_SPEED 5
#define MAX_MONSTER_SPEED 20

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
    int x;
    int y;
    int distance;
} HeapNode;

typedef struct {
    HeapNode *nodes;
    int size;
    int capacity;
} MinHeap;

typedef struct {
    int x;
    int y;
    int speed;
    int alive;
} Player;

typedef struct {
    int x;
    int y;
    int speed;
    char symbol; // 0-f based on characteristics
    int intelligent : 1;
    int telepathic : 1;
    int tunneling : 1;
    int erratic : 1;
    int alive;
    int last_seen_x; // For intelligent monsters
    int last_seen_y;
} Monster;

typedef struct {
    int time; // Event time
    int is_player; // 1 for PC, 0 for monster
    int index; // Index in monster array if applicable
} Event;

extern char dungeon[HEIGHT][WIDTH];
extern unsigned char hardness[HEIGHT][WIDTH];
extern struct Room rooms[MAX_ROOMS];
extern int distance_non_tunnel[HEIGHT][WIDTH];
extern int distance_tunnel[HEIGHT][WIDTH];

extern int player_x;
extern int player_y;
extern int num_rooms;
extern int randRoomNum;
extern int upStairsCount;
extern int downStairsCount;

extern struct Stairs upStairs[MAX_ROOMS];
extern struct Stairs downStairs[MAX_ROOMS];

extern Player pc;
extern Monster *monsters;
extern int num_monsters;

// Dungeon-related functions
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

// MinHeap functions
MinHeap* createMinHeap(int capacity);
void heapify(MinHeap* heap, int idx);
void insertHeap(MinHeap* heap, HeapNode node);
HeapNode extractMin(MinHeap* heap);
void decreasePriority(MinHeap* heap, int x, int y, int newDistance);

// Pathfinding functions
void dijkstraNonTunneling(int dist[HEIGHT][WIDTH]);
void printNonTunnelingMap();
void dijkstraTunneling(int dist[HEIGHT][WIDTH]);
void printTunnelingMap();

// Monster functions
void spawnMonsters(int count);
void moveMonster(int index);
int hasLineOfSight(int x1, int y1, int x2, int y2);
void updateDistanceMaps();
void freeMonsters();

// Game loop functions
void runGame();

#endif