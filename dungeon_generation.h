#ifndef DUNGEON_GENERATION_H
#define DUNGEON_GENERATION_H

#include <ncurses.h>
#include <string>
#include <vector>
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

struct Room {
    int x, y, height, width;
};

struct Stairs {
    int x, y;
};

class Object {
public:
    int x, y;
    std::string name;
    std::vector<std::string> types;
    std::string color;
    char symbol;
    Dice damage;
    int hit, dodge, defense, weight, speed, attribute, value;
    bool is_artifact;

    Object(int x_, int y_);
    ~Object();
};

class Character {
public:
    int x, y;
    int speed;
    int alive;
    int last_seen_x, last_seen_y;
    std::string name;
    std::string color;
    char symbol;
    int hitpoints;
    Dice damage;

    Character(int x_, int y_, int hp = 0);
    virtual ~Character() = default;
    virtual void move() = 0;
};

class PC : public Character {
public:
    PC(int x_, int y_);
    void move() override;
    Object* equipment[12]; // WEAPON, OFFHAND, RANGED, ARMOR, HELMET, CLOAK, GLOVES, BOOTS, AMULET, LIGHT, RING1, RING2
    Object* carry[10];
    int getTotalSpeed() const;
    int rollTotalDamage() const;
};

class NPC : public Character {
public:
    int intelligent, tunneling, telepathic, erratic;
    int pass_wall, pickup, destroy;
    bool is_unique;

    NPC(int x_, int y_);
    void move() override;
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

extern std::vector<MonsterDescription> monsterDescs;
extern std::vector<ObjectDescription> objectDescs;

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
void dijkstraTunneling(int dist[HEIGHT][WIDTH]);
void printNonTunnelingMap();
void printTunnelingMap();

int spawnMonsters(int count);
void runGame(int numMonsters);

int gameOver(NPC** culprit);

void init_ncurses();
void update_visibility();
void draw_dungeon(WINDOW* win, const char* message);
void draw_monster_list(WINDOW* win);
void regenerate_dungeon(int numMonsters);
int move_player(int dx, int dy, const char** message);
int use_stairs(char direction, int numMonsters, const char** message);

void placeObjects(int count);
void cleanupObjects();
void loadDescriptions();

void wear_item(WINDOW* win, const char** message);
void take_off_item(WINDOW* win, const char** message);
void drop_item(WINDOW* win, const char** message);
void expunge_item(WINDOW* win, const char** message);
void list_inventory(WINDOW* win, const char** message);
void list_equipment(WINDOW* win, const char** message);
void inspect_item(WINDOW* win, const char** message);
void look_at_monster(WINDOW* win, const char** message);
void pickup_item(WINDOW* win, const char** message);

#endif