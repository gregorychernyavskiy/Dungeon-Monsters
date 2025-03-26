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
    int x, y;    
    int intelligent;  
    int tunneling;    
    int telepathic;  
    int erratic;
    int speed;     
    int alive;       
    int last_seen_x;
    int last_seen_y;
} Monster;

typedef struct {
    int time;
    Monster *monster;
} Event;

extern char dungeon[HEIGHT][WIDTH];         
extern unsigned char hardness[HEIGHT][WIDTH]; 
extern struct Room rooms[MAX_ROOMS];
extern int distance_non_tunnel[HEIGHT][WIDTH];
extern int distance_tunnel[HEIGHT][WIDTH];
extern Monster *monsterAt[HEIGHT][WIDTH];

extern Monster **monsters;
extern int num_monsters;

extern int player_x;
extern int player_y;
extern int num_rooms; 
extern int randRoomNum;
extern int upStairsCount;
extern int downStairsCount;
extern int player_room_index;

extern char *dungeonFile;

extern struct Stairs upStairs[MAX_ROOMS];
extern struct Stairs downStairs[MAX_ROOMS];

// Add these declarations
extern int fog_enabled;
extern char visible[HEIGHT][WIDTH];
extern char terrain[HEIGHT][WIDTH];

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

Monster *generateMonsterByType(char c, int x, int y);
Monster *generateMonster(int x, int y);
int spawnMonsterByType(char monType);
int spawnMonsters(int count);
void relocateMonster(Monster *monster);
void runGame(int numMonsters);

int gameOver(Monster **culprit);

void movePlayer(void);

// Functions moved from main.c
void init_ncurses(void);
void update_visibility(void);
void draw_dungeon(WINDOW *win, const char *message);
void draw_monster_list(WINDOW *win);
void regenerate_dungeon(int numMonsters);
int move_player(int dx, int dy, const char **message);
int use_stairs(char direction, int numMonsters, const char **message);

#endif