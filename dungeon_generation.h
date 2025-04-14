#ifndef DUNGEON_GENERATION_H
#define DUNGEON_GENERATION_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <time.h>
#include <endian.h>
#include <limits.h>
#include <ncurses.h>
#include <vector>
#include <string>
#include "minheap.h"
#include "dice.h"
#include "monster_parsing.h"
#include "object_parsing.h"

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
#define MIN_OBJECTS 10

struct Room {
    int x, y, height, width;
};

struct Stairs {
    int x, y;
};

class Object {
public:
    std::string name;
    std::string color;
    std::vector<std::string> types;
    int hit;        // Rolled from dice
    Dice damage;    // Remains as dice
    int dodge;      // Rolled
    int defense;    // Rolled
    int weight;     // Rolled
    int speed;      // Rolled
    int attribute;  // Rolled
    int value;      // Rolled
    bool is_artifact;
    int rarity;
    int x, y;

    Object(const ObjectDescription& desc, int x_, int y_);
    void render(WINDOW* win) const;
};

class Character {
public:
    int x, y;
    int speed;
    int alive;
    int last_seen_x, last_seen_y;

    Character(int x_, int y_) : x(x_), y(y_), speed(10), alive(1), last_seen_x(-1), last_seen_y(-1) {}
    virtual ~Character() = default;
    virtual void move() = 0;
    virtual void render(WINDOW* win) const = 0;
};

class PC : public Character {
public:
    PC(int x_, int y_) : Character(x_, y_) {}
    void move() override;
    void render(WINDOW* win) const override;
};

class NPC : public Character {
public:
    std::string name;
    std::string color; // First color
    char symbol;
    int hitpoints;
    Dice damage;
    bool intelligent;
    bool tunneling;
    bool telepathic;
    bool erratic;
    bool pass_wall;
    int rarity;
    bool is_unique;

    NPC(const MonsterDescription& desc, int x_, int y_);
    void move() override;
    void render(WINDOW* win) const override;
};

extern char dungeon[HEIGHT][WIDTH];
extern unsigned char hardness[HEIGHT][WIDTH];
extern struct Room rooms[MAX_ROOMS];
extern int distance_non_tunnel[HEIGHT][WIDTH];
extern int distance_tunnel[HEIGHT][WIDTH];
extern NPC* monsterAt[HEIGHT][WIDTH];
extern Object* objectAt[HEIGHT][WIDTH];

extern NPC** monsters;
extern int num_monsters;
extern Object** objects;
extern int num_objects;

extern PC* player;
extern int num_rooms;
extern int randRoomNum;
extern int upStairsCount;
extern int downStairsCount;
extern int player_room_index;

extern char* dungeonFile;
extern struct Stairs upStairs[MAX_ROOMS];
extern struct Stairs downStairs[MAX_ROOMS];

extern int fog_enabled;
extern char visible[HEIGHT][WIDTH];
extern char terrain[HEIGHT][WIDTH];
extern char remembered[HEIGHT][WIDTH];

extern std::vector<MonsterDescription> monster_descriptions;
extern std::vector<ObjectDescription> object_descriptions;
extern std::vector<std::string> spawned_artifacts;
extern std::vector<std::string> spawned_unique_monsters;

void printDungeon();
void emptyDungeon();
int overlapCheck(struct Room r1, struct Room r2);
void createRooms();
void connectRooms();
void placeStairs();
void placePlayer();
void initializeHardness();
void printHardness();
void saveDungeon(char* filename);
void loadDungeon(char* filename);

void dijkstraNonTunneling(int dist[HEIGHT][WIDTH]);
void printNonTunnelingMap();
void dijkstraTunneling(int dist[HEIGHT][WIDTH]);
void printTunnelingMap();

NPC* generateMonsterByType(char c, int x, int y);
NPC* generateMonster(int x, int y);
int spawnMonsterByType(char monType);
int spawnMonsters(int count);
int spawnObjects();
void runGame(int numMonsters);

int gameOver(NPC** culprit);

void init_ncurses();
void update_visibility();
void draw_dungeon(WINDOW* win, const char* message);
void draw_monster_list(WINDOW* win);
void regenerate_dungeon(int numMonsters);
int move_player(int dx, int dy, const char** message);
int use_stairs(char direction, int numMonsters, const char** message);

#endif