#ifndef DUNGEON_GENERATION_H
#define DUNGEON_GENERATION_H

#include <ncurses.h>
#include <string>
#include <vector>
#include <map>
#include <queue>
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
#define POISON_BALL_RADIUS 3
#define POISON_BALL_MANA_COST 20
#define POISON_BALL_DAMAGE Dice(0, 1, 20)
#define RANGED_ATTACK_RANGE 5
#define POISON_DURATION 10
#define POISON_TICK_DAMAGE Dice(0, 1, 12)

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
    Dice heal;
    int hit, dodge, defense, weight, speed, attribute, value;
    int range;
    bool is_artifact;

    Object(int x_, int y_);
    ~Object();
};

class Character {
public:
    int x, y;
    int speed;
    int hitpoints;
    Dice damage;
    int alive;
    int last_seen_x, last_seen_y;
    std::string name;
    std::string color;
    char symbol;

    Character(int x_, int y_);
    virtual ~Character() = default;
    virtual void move() = 0;
    virtual int takeDamage(int damage);
};

class PC : public Character {
public:
    static const int CARRY_SLOTS = 10;
    static const int EQUIPMENT_SLOTS = 13;
    Object* equipment[EQUIPMENT_SLOTS];
    Object* carry[CARRY_SLOTS];
    int num_carried;
    int mana;
    int max_mana;

    PC(int x_, int y_);
    ~PC();
    void move() override;
    int takeDamage(int damage) override;
    bool pickupObject(Object* obj);
    void calculateStats(int& total_speed, Dice& total_damage, int& total_defense, int& total_hit, int& total_dodge);
    void heal(int amount);
};

class NPC : public Character {
public:
        int intelligent, tunneling, telepathic, erratic;
        int pass_wall, pickup, destroy;
        bool is_unique;
        bool is_boss;
        bool is_poisoned;
        int poison_turns_remaining;
        Dice poison_damage;
    
        NPC(int x_, int y_);
        void move() override;
        int takeDamage(int damage) override;
        bool displace(int& new_x, int& new_y);
        void applyPoison(Dice poison_dmg, int duration);
};
    
struct Event {
        int64_t time;
        Character* character;
        enum EventType { MOVE, POISON_TICK } type;
        Event(int64_t t, Character* c, EventType et = MOVE) : time(t), character(c), type(et) {}
        bool operator>(const Event& other) const { return time > other.time; }
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

extern NPC* engaged_monster;
extern bool in_combat;

extern int current_level;
extern std::map<int, std::vector<Object*>> level_objects;

extern std::vector<MonsterDescription> monsterDescs;

extern std::priority_queue<Event, std::vector<Event>, std::greater<Event>> event_queue;
extern int64_t game_turn;

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
void schedule_event(Character* character);

int gameOver(NPC** culprit);

void init_ncurses();
void update_visibility();
void draw_dungeon(WINDOW* win, const char* message);
void draw_monster_list(WINDOW* win);
void regenerate_dungeon(int numMonsters);
int move_player(int dx, int dy, const char** message);
int use_stairs(char direction, int numMonsters, const char** message);
int fight_monster(WINDOW* win, NPC* monster, int ch, const char** message);
int fire_ranged_weapon(int target_x, int target_y, const char** message);
int cast_poison_ball(int target_x, int target_y, const char** message);

void placeObjects(int count);
void cleanupObjects();
void loadDescriptions();

enum EquipmentSlot {
    SLOT_WEAPON, SLOT_OFFHAND, SLOT_RANGED, SLOT_ARMOR, SLOT_HELMET, SLOT_CLOAK,
    SLOT_GLOVES, SLOT_BOOTS, SLOT_AMULET, SLOT_LIGHT, SLOT_RING1, SLOT_RING2, SLOT_SPELLBOOK
};

void display_inventory(WINDOW* win, PC* pc, const char** message);
void display_equipment(WINDOW* win, PC* pc, const char** message);
void display_stats(WINDOW* win, PC* pc, const char** message);
void display_help(WINDOW* win, const char** message);
void display_monster_info(WINDOW* win, NPC* monster, const char** message);
void wear_item(WINDOW* win, PC* pc, const char** message);
void take_off_item(WINDOW* win, PC* pc, const char** message);
void drop_item(WINDOW* win, PC* pc, const char** message);
void expunge_item(WINDOW* win, PC* pc, const char** message);
void inspect_item(WINDOW* win, PC* pc, const char** message);
void use_item(WINDOW* win, PC* pc, const char** message);
void apply_poison_tick(NPC* monster, const char** message);

#endif