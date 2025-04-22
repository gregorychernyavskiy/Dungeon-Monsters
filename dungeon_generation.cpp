#include "dungeon_generation.h"
#include <unistd.h>
#include <ctype.h>
#include <ncurses.h>
#include <random>
#include <ctime>
#include <cstring>
#include <cstdlib>

char dungeon[HEIGHT][WIDTH];
unsigned char hardness[HEIGHT][WIDTH];
struct Room rooms[MAX_ROOMS];
int distance_non_tunnel[HEIGHT][WIDTH];
int distance_tunnel[HEIGHT][WIDTH];
NPC* monsterAt[HEIGHT][WIDTH] = {nullptr};
Object* objectAt[HEIGHT][WIDTH] = {nullptr};

NPC** monsters = nullptr;
int num_monsters = 0;
Object** objects = nullptr;
int num_objects = 0;

PC* player = nullptr;
int num_rooms = 0;
int randRoomNum = 0;
int upStairsCount = 0;
int downStairsCount = 0;
int player_room_index = -1;

char* dungeonFile = nullptr;
struct Stairs upStairs[MAX_ROOMS];
struct Stairs downStairs[MAX_ROOMS];

int fog_enabled = 1;
char visible[HEIGHT][WIDTH] = {0};
char terrain[HEIGHT][WIDTH] = {0};
char remembered[HEIGHT][WIDTH] = {0};

NPC* engaged_monster = nullptr;
bool in_combat = false;

std::vector<MonsterDescription> monsterDescs;
std::vector<ObjectDescription> objectDescs;

Object::Object(int x_, int y_) : x(x_), y(y_), hit(0), dodge(0), defense(0),
    weight(0), speed(0), attribute(0), value(0), is_artifact(false) {}

Object::~Object() {}

Character::Character(int x_, int y_) : x(x_), y(y_), speed(10), hitpoints(100), alive(1),
    last_seen_x(-1), last_seen_y(-1) {
    damage = Dice(0, 1, 4); // Default damage: 0+1d4
}

int Character::takeDamage(int damage) {
    hitpoints -= damage;
    if (hitpoints <= 0) {
        alive = 0;
        return 1; // Character died
    }
    return 0; // Character survived
}

PC::PC(int x_, int y_) : Character(x_, y_) {
    symbol = '@';
    color = "WHITE";
    hitpoints = 100; // Default PC hitpoints
    num_carried = 0;
    for (int i = 0; i < EQUIPMENT_SLOTS; i++) equipment[i] = nullptr;
    for (int i = 0; i < CARRY_SLOTS; i++) carry[i] = nullptr;
}

PC::~PC() {
    for (int i = 0; i < EQUIPMENT_SLOTS; i++) delete equipment[i];
    for (int i = 0; i < CARRY_SLOTS; i++) delete carry[i];
}

int PC::takeDamage(int damage) {
    hitpoints -= damage;
    if (hitpoints <= 0) {
        alive = 0;
        return 1; // PC died
    }
    return 0; // PC survived
}

NPC::NPC(int x_, int y_) : Character(x_, y_), intelligent(0),
    tunneling(0), telepathic(0), erratic(0),
    pass_wall(0), pickup(0), destroy(0), is_unique(false), is_boss(false) {
    speed = rand() % 16 + 5;
}

bool PC::pickupObject(Object* obj) {
    if (num_carried >= CARRY_SLOTS) return false;
    for (int i = 0; i < CARRY_SLOTS; i++) {
        if (!carry[i]) {
            carry[i] = obj;
            num_carried++;
            return true;
        }
    }
    return false;
}

void PC::calculateStats(int& total_speed, Dice& total_damage) {
    total_speed = speed; // Base speed: 10
    total_damage = damage; // Start with base damage (0+1d4)

    // Add equipment bonuses, but only override damage if a weapon is equipped
    for (int i = 0; i < EQUIPMENT_SLOTS; i++) {
        if (equipment[i]) {
            total_speed += equipment[i]->speed;
            if (i == SLOT_WEAPON) {
                total_damage = Dice(0, 0, 0); // Reset damage if weapon is equipped, then add weapon damage
                total_damage.base += equipment[i]->damage.base;
                total_damage.dice += equipment[i]->damage.dice;
                total_damage.sides = std::max(total_damage.sides, equipment[i]->damage.sides);
            } else {
                total_damage.base += equipment[i]->damage.base;
                total_damage.dice += equipment[i]->damage.dice;
                total_damage.sides = std::max(total_damage.sides, equipment[i]->damage.sides);
            }
        }
    }
}

bool NPC::displace(int& new_x, int& new_y) {
    int dx[] = {-1, 0, 1, -1, 1, -1, 0, 1};
    int dy[] = {-1, -1, -1, 0, 0, 1, 1, 1};
    std::vector<std::pair<int, int>> open_cells;

    for (int i = 0; i < 8; i++) {
        int nx = x + dx[i];
        int ny = y + dy[i];
        if (nx >= 0 && nx < WIDTH && ny >= 0 && ny < HEIGHT &&
            hardness[ny][nx] == 0 && !monsterAt[ny][nx] && !(nx == player->x && ny == player->y)) {
            open_cells.emplace_back(nx, ny);
        }
    }

    if (!open_cells.empty()) {
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<> dis(0, open_cells.size() - 1);
        int idx = dis(gen);
        new_x = open_cells[idx].first;
        new_y = open_cells[idx].second;
        return true;
    }

    return false;
}

void PC::move() {}

void compactMonsters() {
    int write_idx = 0;
    for (int i = 0; i < num_monsters; i++) {
        if (monsters[i] != nullptr) {
            monsters[write_idx] = monsters[i];
            write_idx++;
        }
    }
    num_monsters = write_idx;
    NPC** temp = (NPC**)realloc(monsters, num_monsters * sizeof(NPC*));
    if (temp) {
        monsters = temp;
    }
}

void NPC::move() {
    if (!alive || in_combat) return;
    int dist[HEIGHT][WIDTH];
    if (tunneling || pass_wall) dijkstraTunneling(dist);
    else dijkstraNonTunneling(dist);
    int curr_x = x, curr_y = y;
    int min_dist = dist[curr_y][curr_x];
    int next_x = curr_x, next_y = curr_y;

    int dx[] = {-1, 0, 1, -1, 1, -1, 0, 1};
    int dy[] = {-1, -1, -1, 0, 0, 1, 1, 1};

    if (erratic && rand() % 2) {
        int i = rand() % 8;
        next_x = curr_x + dx[i];
        next_y = curr_y + dy[i];
    } else {
        for (int i = 0; i < 8; i++) {
            int nx = curr_x + dx[i];
            int ny = curr_y + dy[i];
            if (nx >= 0 && nx < WIDTH && ny >= 0 && ny < HEIGHT) {
                if (dist[ny][nx] < min_dist && (tunneling || pass_wall || hardness[ny][nx] == 0)) {
                    min_dist = dist[ny][nx];
                    next_x = nx;
                    next_y = ny;
                }
            }
        }
    }

    if (next_x != curr_x || next_y != curr_y) {
        if (next_x == player->x && next_y == player->y) {
            engaged_monster = this;
            in_combat = true;
            return;
        }
        if (monsterAt[next_y][next_x]) {
            int disp_x, disp_y;
            NPC* other = monsterAt[next_y][next_x];
            if (other->displace(disp_x, disp_y)) {
                monsterAt[other->y][other->x] = nullptr;
                monsterAt[disp_y][disp_x] = other;
                other->x = disp_x;
                other->y = disp_y;
            } else {
                monsterAt[curr_y][curr_x] = other;
                other->x = curr_x;
                other->y = curr_y;
            }
        }
        if (hardness[next_y][next_x] > 0 && tunneling && !pass_wall) {
            hardness[next_y][next_x] = 0;
            dungeon[next_y][next_x] = '#';
            terrain[next_y][next_x] = '#';
        }
        if (objectAt[next_y][next_x]) {
            if (destroy) {
                for (int i = 0; i < num_objects; ++i) {
                    if (objects[i] && objects[i]->x == next_x && objects[i]->y == next_y) {
                        delete objects[i];
                        objects[i] = nullptr;
                        objectAt[next_y][next_x] = nullptr;
                        break;
                    }
                }
            } else if (pickup) {
                // Optional: Implement NPC inventory later
            }
        }
        monsterAt[curr_y][curr_x] = nullptr;
        x = next_x;
        y = next_y;
        monsterAt[next_y][next_x] = this;
    }
}

int NPC::takeDamage(int damage) {
    hitpoints -= damage;
    if (hitpoints <= 0) {
        alive = 0;
        return 1; // NPC died
    }
    return 0;
}

int fight_monster(WINDOW* win, NPC* monster, int ch, const char** message) {
    static bool player_turn = true;
    static bool combat_initialized = false;
    static char combat_message[80] = "";

    if (!combat_initialized) {
        player_turn = true;
        combat_initialized = true;
        snprintf(combat_message, sizeof(combat_message), "Combat started!");
    }

    if (ch == 'a' && player_turn) {
        int total_speed;
        Dice total_damage;
        player->calculateStats(total_speed, total_damage);
        std::random_device rd;
        std::mt19937 gen(rd());
        int damage = total_damage.base;
        for (int i = 0; i < total_damage.dice; i++) {
            std::uniform_int_distribution<> dis(1, total_damage.sides);
            damage += dis(gen);
        }
        snprintf(combat_message, sizeof(combat_message), "You deal %d damage to %s!", damage, monster->name.c_str());
        if (monster->takeDamage(damage)) {
            monsterAt[monster->y][monster->x] = nullptr;
            for (int i = 0; i < num_monsters; i++) {
                if (monsters[i] == monster) {
                    if (monster->is_unique) {
                        for (auto& desc : monsterDescs) {
                            if (desc.name == monster->name) desc.is_alive = false;
                        }
                    }
                    delete monsters[i];
                    monsters[i] = nullptr;
                    break;
                }
            }
            compactMonsters();
            in_combat = false;
            combat_initialized = false;
            engaged_monster = nullptr;
            if (monster->is_boss) {
                *message = "You defeated SpongeBob SquarePants! You win!";
                return -1;
            }
            *message = "You defeated the monster!";
            return 0;
        }
        player_turn = false;
    } else if (ch == 'f' && player_turn) {
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<> dis(1, 100);
        int flee_chance = 30;
        if (dis(gen) <= flee_chance) {
            in_combat = false;
            combat_initialized = false;
            engaged_monster = nullptr;
            *message = "You fled from the monster!";
            return 0;
        } else {
            snprintf(combat_message, sizeof(combat_message), "Failed to flee!");
            player_turn = false;
        }
    } else if (!player_turn) {
        std::random_device rd;
        std::mt19937 gen(rd());
        int damage = monster->damage.base;
        for (int i = 0; i < monster->damage.dice; i++) {
            std::uniform_int_distribution<> dis(1, monster->damage.sides);
            damage += dis(gen);
        }
        snprintf(combat_message, sizeof(combat_message), "%s deals %d damage to you!", monster->name.c_str(), damage);
        if (player->takeDamage(damage)) {
            in_combat = false;
            combat_initialized = false;
            engaged_monster = nullptr;
            *message = "You were killed! Game over!";
            return -2;
        }
        player_turn = true;
    } else {
        snprintf(combat_message, sizeof(combat_message), player_turn ? "Press 'a' to attack, 'f' to flee" : "Monster's turn...");
    }

    werase(win);
    mvwprintw(win, 0, 0, "Combat Mode (Press 'a' to attack, 'f' to flee):");
    mvwprintw(win, 1, 0, "Player HP: %d", player->hitpoints);
    mvwprintw(win, 2, 0, "Monster: %s (%c) HP: %d", monster->name.c_str(), monster->symbol, monster->hitpoints);
    mvwprintw(win, 3, 0, "%s", combat_message);
    mvwprintw(win, 4, 0, "Turn: %s", player_turn ? "Player" : "Monster");
    wrefresh(win);
    return 1;
}

void printDungeon() {
    for (int y = 0; y < HEIGHT; y++) {
        for (int x = 0; x < WIDTH; x++) {
            if (monsterAt[y][x]) {
                printf("%c", monsterAt[y][x]->symbol);
            } else if (x == player->x && y == player->y) {
                printf("@");
            } else if (objectAt[y][x]) {
                printf("%c", objectAt[y][x]->symbol);
            } else {
                printf("%c", dungeon[y][x]);
            }
        }
        printf("\n");
    }
}

void emptyDungeon() {
    for (int y = 0; y < HEIGHT; y++) {
        for (int x = 0; x < WIDTH; x++) {
            dungeon[y][x] = (x == 0 || x == WIDTH - 1 || y == 0 || y == HEIGHT - 1) ? '+' : ' ';
        }
    }
}

int overlapCheck(struct Room r1, struct Room r2) {
    return !(r1.x + r1.width < r2.x || r2.x + r2.width < r1.x || 
             r1.y + r1.height < r2.y || r2.y + r2.height < r1.y);
}

void createRooms() {
    num_rooms = 0;
    int attempts = 0;
    randRoomNum = MIN_ROOMS + rand() % (MAX_ROOMS - MIN_ROOMS + 1);

    while (num_rooms < randRoomNum && attempts < 1000) {
        attempts++;
        struct Room newRoom;
        newRoom.width = MIN_ROOM_WIDTH + rand() % (MAX_ROOM_WIDTH - MIN_ROOM_WIDTH + 1);
        newRoom.height = MIN_ROOM_HEIGHT + rand() % (MAX_ROOM_HEIGHT - MIN_ROOM_HEIGHT + 1);
        newRoom.x = 1 + rand() % (WIDTH - newRoom.width - 2);
        newRoom.y = 1 + rand() % (HEIGHT - newRoom.height - 2);

        bool overlap = false;
        for (int i = 0; i < num_rooms; i++) {
            if (overlapCheck(newRoom, rooms[i])) {
                overlap = true;
                break;
            }
        }
        if (!overlap) {
            rooms[num_rooms++] = newRoom;
            for (int y = newRoom.y; y < newRoom.y + newRoom.height; y++) {
                for (int x = newRoom.x; x < newRoom.x + newRoom.width; x++) {
                    dungeon[y][x] = '.';
                }
            }
        }
    }
}

void connectRooms() {
    for (int i = 0; i < num_rooms - 1; i++) {
        int x1 = rooms[i].x + rooms[i].width / 2;
        int y1 = rooms[i].y + rooms[i].height / 2;
        int x2 = rooms[i + 1].x + rooms[i + 1].width / 2;
        int y2 = rooms[i + 1].y + rooms[i + 1].height / 2;

        while (x1 != x2 || y1 != y2) {
            if (dungeon[y1][x1] == ' ') dungeon[y1][x1] = '#';
            if (rand() % 2) {
                if (x1 < x2) x1++;
                else if (x1 > x2) x1--;
            } else {
                if (y1 < y2) y1++;
                else if (y1 > y2) y1--;
            }
        }
    }
}

void placeStairs() {
    upStairsCount = 1;
    downStairsCount = 1;
    int upIndex = rand() % num_rooms;
    int downIndex = rand() % num_rooms;
    while (downIndex == upIndex) downIndex = rand() % num_rooms;

    struct Room upRoom = rooms[upIndex];
    struct Room downRoom = rooms[downIndex];

    upStairs[0].x = upRoom.x + rand() % upRoom.width;
    upStairs[0].y = upRoom.y + rand() % upRoom.height;
    dungeon[upStairs[0].y][upStairs[0].x] = '<';

    downStairs[0].x = downRoom.x + rand() % downRoom.width;
    downStairs[0].y = downRoom.y + rand() % downRoom.height;
    dungeon[downStairs[0].y][downStairs[0].x] = '>';
}

void initializeHardness() {
    for (int y = 0; y < HEIGHT; y++) {
        for (int x = 0; x < WIDTH; x++) {
            if (x == 0 || x == WIDTH - 1 || y == 0 || y == HEIGHT - 1) {
                hardness[y][x] = 255;
            } else if (dungeon[y][x] == '.' || dungeon[y][x] == '#' || dungeon[y][x] == '@' ||
                       dungeon[y][x] == '<' || dungeon[y][x] == '>') {
                hardness[y][x] = 0;
            } else {
                hardness[y][x] = (rand() % 254) + 1;
            }
        }
    }
}

void printHardness() {
    for (int y = 0; y < HEIGHT; y++) {
        for (int x = 0; x < WIDTH; x++) {
            printf("%3d ", hardness[y][x]);
        }
        printf("\n");
    }
}

void placePlayer() {
    int index = rand() % num_rooms;
    player_room_index = index;
    struct Room playerRoom = rooms[index];
    int px = playerRoom.x + rand() % playerRoom.width;
    int py = playerRoom.y + rand() % playerRoom.height;
    if (player) delete player;
    player = new PC(px, py);
    dungeon[player->y][player->x] = '@';
}

void loadDescriptions() {
    char* home = getenv("HOME");
    if (!home) {
        fprintf(stderr, "Error: HOME environment variable not set\n");
        exit(EXIT_FAILURE);
    }
    std::string monsterFile = std::string(home) + "/.rlg327/monster_desc.txt";
    std::string objectFile = std::string(home) + "/.rlg327/object_desc.txt";
    monsterDescs = parseMonsterDescriptions(monsterFile);
    objectDescs = parseObjectDescriptions(objectFile);
}

int spawnMonsters(int numMonsters) {
    if (monsters) {
        for (int i = 0; i < num_monsters; ++i) {
            if (monsters[i] && monsterAt[monsters[i]->y][monsters[i]->x]) {
                monsterAt[monsters[i]->y][monsters[i]->x] = nullptr;
            }
            if (monsters[i]->is_unique && !monsters[i]->alive) {
                for (auto& desc : monsterDescs) {
                    if (desc.name == monsters[i]->name) {
                        desc.is_alive = false;
                    }
                }
            }
            delete monsters[i];
        }
        free(monsters);
        monsters = nullptr;
    }
    num_monsters = 0;
    monsters = (NPC**)malloc(numMonsters * sizeof(NPC*));
    if (!monsters) {
        fprintf(stderr, "Error: Failed to allocate memory for monsters\n");
        return 1;
    }

    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, 99);
    std::uniform_int_distribution<> desc_dis(0, monsterDescs.size() - 1);

    for (int i = 0; i < numMonsters; ++i) {
        NPC* npc = nullptr;
        int attempts = 100;
        while (attempts--) {
            int idx = desc_dis(gen);
            MonsterDescription& desc = monsterDescs[idx];
            if (desc.is_unique && desc.is_alive) continue;
            if (desc.rarity <= dis(gen)) continue;

            int x, y;
            int place_attempts = 100;
            bool placed = false;
            while (place_attempts--) {
                x = rand() % WIDTH;
                y = rand() % HEIGHT;
                struct Room playerRoom = rooms[player_room_index];
                bool in_player_room = (x >= playerRoom.x && x < playerRoom.x + playerRoom.width &&
                                      y >= playerRoom.y && y < playerRoom.y + playerRoom.height);
                if (dungeon[y][x] == '.' && !(x == player->x && y == player->y) && !monsterAt[y][x] && !in_player_room) {
                    npc = desc.createNPC(x, y);
                    npc->is_boss = (desc.name == "SpongeBob SquarePants");
                    placed = true;
                    break;
                }
            }
            if (placed) break;
        }
        if (npc) {
            NPC** temp = (NPC**)realloc(monsters, (num_monsters + 1) * sizeof(NPC*));
            if (temp) {
                monsters = temp;
                monsters[num_monsters] = npc;
                monsterAt[npc->y][npc->x] = npc;
                num_monsters++;
            }
        }
    }
    return num_monsters < numMonsters;
}

void placeObjects(int count) {
    cleanupObjects();
    objects = (Object**)malloc(count * sizeof(Object*));
    num_objects = 0;

    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, 99);
    std::uniform_int_distribution<> desc_dis(0, objectDescs.size() - 1);

    for (int i = 0; i < count; ++i) {
        Object* obj = nullptr;
        int attempts = 100;
        while (attempts--) {
            int idx = desc_dis(gen);
            ObjectDescription& desc = objectDescs[idx];
            if (desc.is_artifact && desc.is_created) continue;
            if (desc.rarity <= dis(gen)) continue;

            int x, y;
            int place_attempts = 100;
            bool placed = false;
            while (place_attempts--) {
                x = rand() % WIDTH;
                y = rand() % HEIGHT;
                struct Room playerRoom = rooms[player_room_index];
                bool in_player_room = (x >= playerRoom.x && x < playerRoom.x + playerRoom.width &&
                                      y >= playerRoom.y && y < playerRoom.y + playerRoom.height);
                if (dungeon[y][x] == '.' && !monsterAt[y][x] && !objectAt[y][x] && !in_player_room &&
                    !(x == player->x && y == player->y)) {
                    obj = desc.createObject(x, y);
                    placed = true;
                    break;
                }
            }
            if (placed) break;
        }
        if (obj) {
            Object** temp = (Object**)realloc(objects, (num_objects + 1) * sizeof(Object*));
            if (temp) {
                objects = temp;
                objects[num_objects] = obj;
                objectAt[obj->y][obj->x] = obj;
                num_objects++;
            }
        }
    }
}

void cleanupObjects() {
    for (int i = 0; i < num_objects; ++i) {
        if (objects[i]) {
            objectAt[objects[i]->y][objects[i]->x] = nullptr;
            delete objects[i];
        }
    }
    free(objects);
    objects = nullptr;
    num_objects = 0;
}

void runGame(int numMonsters) {
    if (spawnMonsters(numMonsters)) {
        printf("Failed to spawn monsters\n");
        return;
    }
    int turns = 0;
    int monsters_alive = num_monsters;

    while (monsters_alive > 0) {
        printf("\nTurn %d:\n", turns++);
        printDungeon();
        monsters_alive = 0;
        for (int i = 0; i < num_monsters; i++) {
            if (monsters[i]->alive) {
                monsters[i]->move();
                if (monsters[i]->alive) monsters_alive++;
            }
        }
        sleep(1);
    }
    printf("\nFinal state:\n");
    printDungeon();
}

int gameOver(NPC** culprit) {
    for (int i = 0; i < num_monsters; i++) {
        if (monsters[i] && monsters[i]->alive && monsters[i]->x == player->x && monsters[i]->y == player->y) {
            *culprit = monsters[i];
            return 1;
        }
    }
    *culprit = nullptr;
    return 0;
}

void init_ncurses() {
    initscr();
    if (!has_colors()) {
        endwin();
        fprintf(stderr, "Error: Terminal does not support colors!\n");
        exit(EXIT_FAILURE);
    }
    int ret = start_color();
    if (ret == ERR) {
        endwin();
        fprintf(stderr, "Error: Failed to start color mode!\n");
        exit(EXIT_FAILURE);
    }

    raw();
    noecho();
    keypad(stdscr, TRUE);
    curs_set(0);

    init_pair(COLOR_RED, COLOR_RED, COLOR_BLACK);
    init_pair(COLOR_GREEN, COLOR_GREEN, COLOR_BLACK);
    init_pair(COLOR_YELLOW, COLOR_YELLOW, COLOR_BLACK);
    init_pair(COLOR_BLUE, COLOR_BLUE, COLOR_BLACK);
    init_pair(COLOR_MAGENTA, COLOR_MAGENTA, COLOR_BLACK);
    init_pair(COLOR_CYAN, COLOR_CYAN, COLOR_BLACK);
    init_pair(COLOR_WHITE, COLOR_WHITE, COLOR_BLACK);

    FILE* debug_file = fopen("color_init_debug.txt", "w");
    if (debug_file) {
        fprintf(debug_file, "Initialized color pairs:\n");
        fprintf(debug_file, "RED: %d\n", COLOR_RED);
        fprintf(debug_file, "GREEN: %d\n", COLOR_GREEN);
        fprintf(debug_file, "YELLOW: %d\n", COLOR_YELLOW);
        fprintf(debug_file, "BLUE: %d\n", COLOR_BLUE);
        fprintf(debug_file, "MAGENTA: %d\n", COLOR_MAGENTA);
        fprintf(debug_file, "CYAN: %d\n", COLOR_CYAN);
        fprintf(debug_file, "WHITE: %d\n", COLOR_WHITE);
        fclose(debug_file);
    }

    refresh();
    getch();
}

int getColorIndex(const std::string& color) {
    std::string trimmed_color = color;
    trimmed_color.erase(0, trimmed_color.find_first_not_of(" \t"));
    trimmed_color.erase(trimmed_color.find_last_not_of(" \t") + 1);

    std::string upper_color = trimmed_color;
    for (char& c : upper_color) {
        c = std::toupper(c);
    }

    FILE* debug_file = fopen("color_debug.txt", "a");
    if (debug_file) {
        fprintf(debug_file, "Raw color: '%s', Trimmed color: '%s', Mapped color: '%s'\n", 
                color.c_str(), trimmed_color.c_str(), upper_color.c_str());
        fclose(debug_file);
    }

    if (upper_color == "RED") return COLOR_RED;
    if (upper_color == "GREEN") return COLOR_GREEN;
    if (upper_color == "YELLOW") return COLOR_YELLOW;
    if (upper_color == "BLUE") return COLOR_BLUE;
    if (upper_color == "MAGENTA") return COLOR_MAGENTA;
    if (upper_color == "CYAN") return COLOR_CYAN;
    if (upper_color == "WHITE") return COLOR_WHITE;
    if (upper_color == "BLACK") return COLOR_WHITE;
    return COLOR_WHITE;
}

void update_visibility() {
    const int radius = 3;
    memset(visible, 0, sizeof(visible));
    for (int y = player->y - radius; y <= player->y + radius; y++) {
        for (int x = player->x - radius; x <= player->x + radius; x++) {
            if (x >= 0 && x < WIDTH && y >= 0 && y < HEIGHT) {
                int dx = x - player->x;
                int dy = y - player->y;
                if (dx * dx + dy * dy <= radius * radius) {
                    visible[y][x] = 1;
                    remembered[y][x] = terrain[y][x] ? terrain[y][x] : dungeon[y][x];
                }
            }
        }
    }
}

void draw_dungeon(WINDOW* win, const char* message) {
    if (in_combat && engaged_monster) {
        return;
    }

    werase(win);
    mvwprintw(win, 0, 0, "%s", message ? message : "");

    for (int y = 0; y < HEIGHT; y++) {
        for (int x = 0; x < WIDTH; x++) {
            if (fog_enabled && !visible[y][x] && !remembered[y][x]) {
                mvwprintw(win, y + 1, x, " ");
            } else if (x == player->x && y == player->y) {
                int color = getColorIndex(player->color);
                FILE* debug_file = fopen("render_debug.txt", "a");
                if (debug_file) {
                    fprintf(debug_file, "Player at (%d,%d) color index: %d\n", x, y, color);
                    fclose(debug_file);
                }
                wattron(win, COLOR_PAIR(color));
                mvwprintw(win, y + 1, x, "@");
                wattroff(win, COLOR_PAIR(color));
            } else if (monsterAt[y][x] && (!fog_enabled || visible[y][x])) {
                int color = getColorIndex(monsterAt[y][x]->color);
                FILE* debug_file = fopen("render_debug.txt", "a");
                if (debug_file) {
                    fprintf(debug_file, "Monster at (%d,%d) symbol: %c, color index: %d\n", 
                            x, y, monsterAt[y][x]->symbol, color);
                    fclose(debug_file);
                }
                wattron(win, COLOR_PAIR(color));
                mvwprintw(win, y + 1, x, "%c", monsterAt[y][x]->symbol);
                wattroff(win, COLOR_PAIR(color));
            } else if (objectAt[y][x] && (!fog_enabled || visible[y][x])) {
                int color = getColorIndex(objectAt[y][x]->color);
                FILE* debug_file = fopen("render_debug.txt", "a");
                if (debug_file) {
                    fprintf(debug_file, "Object at (%d,%d) symbol: %c, color index: %d\n", 
                            x, y, objectAt[y][x]->symbol, color);
                    fclose(debug_file);
                }
                wattron(win, COLOR_PAIR(color));
                mvwprintw(win, y + 1, x, "%c", objectAt[y][x]->symbol);
                wattroff(win, COLOR_PAIR(color));
            } else {
                char display = (fog_enabled && remembered[y][x] && !visible[y][x]) ? remembered[y][x] : dungeon[y][x];
                mvwprintw(win, y + 1, x, "%c", display);
            }
        }
    }
    int total_speed;
    Dice total_damage;
    player->calculateStats(total_speed, total_damage);
    mvwprintw(win, 22, 0, "HP: %d  SPEED: %d", player->hitpoints, total_speed);
    mvwprintw(win, 23, 0, "Fog of War: %s", fog_enabled ? "ON" : "OFF");
    wrefresh(win);
}

void draw_monster_list(WINDOW* win) {
    werase(win);
    int start = 0;
    int max_lines = 22;
    int ch;

    while (true) {
        werase(win);
        mvwprintw(win, 0, 0, "Monster List (Press ESC to exit):");
        for (int i = start; i < num_monsters && i - start < max_lines - 1; i++) {
            if (monsters[i]->alive) {
                int dx = monsters[i]->x - player->x;
                int dy = monsters[i]->y - player->y;
                const char* ns = dy < 0 ? "north" : "south";
                const char* ew = dx < 0 ? "west" : "east";
                mvwprintw(win, i - start + 1, 0, "%c (%s), %d %s and %d %s", 
                          monsters[i]->symbol, monsters[i]->name.c_str(), abs(dy), ns, abs(dx), ew);
            }
        }
        wrefresh(win);

        ch = getch();
        if (ch == 27) break;
        else if (ch == KEY_UP && start > 0) start--;
        else if (ch == KEY_DOWN && start + max_lines - 1 < num_monsters) start++;
    }
}

void regenerate_dungeon(int numMonsters) {
    for (int i = 0; i < num_monsters; i++) {
        if (monsters[i]) {
            if (monsterAt[monsters[i]->y][monsters[i]->x] == monsters[i]) {
                monsterAt[monsters[i]->y][monsters[i]->x] = nullptr;
            }
            if (monsters[i]->is_unique && !monsters[i]->alive) {
                for (auto& desc : monsterDescs) {
                    if (desc.name == monsters[i]->name) {
                        desc.is_alive = false;
                    }
                }
            }
            delete monsters[i];
        }
    }
    free(monsters);
    monsters = nullptr;
    num_monsters = 0;

    cleanupObjects();

    if (player->x >= 0 && player->y >= 0 && player->x < WIDTH && player->y < HEIGHT) {
        dungeon[player->y][player->x] = terrain[player->y][player->x] == 0 ? '.' : terrain[player->y][player->x];
    }

    emptyDungeon();
    createRooms();
    connectRooms();
    placeStairs();

    for (int y = 0; y < HEIGHT; y++) {
        for (int x = 0; x < WIDTH; x++) {
            terrain[y][x] = dungeon[y][x];
        }
    }
    placePlayer();
    initializeHardness();
    spawnMonsters(numMonsters);
    placeObjects(10);

    memset(visible, 0, sizeof(visible));
    memset(remembered, 0, sizeof(remembered));
    update_visibility();
}

int move_player(int dx, int dy, const char** message) {
    if (in_combat) {
        *message = "You are in combat! Press 'a' to attack, 'f' to flee.";
        return 0;
    }

    int new_x = player->x + dx;
    int new_y = player->y + dy;
    if (new_x < 0 || new_x >= WIDTH || new_y < 0 || new_y >= HEIGHT) {
        *message = "Edge of dungeon!";
        return 0;
    }
    if (hardness[new_y][new_x] > 0) {
        *message = "There's a wall in the way!";
        return 0;
    }
    if (monsterAt[new_y][new_x]) {
        engaged_monster = monsterAt[new_y][new_x];
        in_combat = true;
        *message = "Combat started! Press 'a' to attack, 'f' to flee.";
        return 0;
    }
    if (objectAt[new_y][new_x]) {
        Object* obj = objectAt[new_y][new_x];
        if (player->pickupObject(obj)) {
            objectAt[new_y][new_x] = nullptr;
            for (int i = 0; i < num_objects; i++) {
                if (objects[i] == obj) {
                    objects[i] = nullptr;
                    break;
                }
            }
            *message = "Picked up an item!";
        } else {
            *message = "Inventory full!";
        }
    }
    dungeon[player->y][player->x] = terrain[player->y][player->x];
    if (objectAt[player->y][player->x]) {
        terrain[player->y][player->x] = objectAt[player->y][player->x]->symbol;
    }
    player->x = new_x;
    player->y = new_y;
    if (terrain[player->y][player->x] == 0) {
        terrain[player->y][player->x] = dungeon[player->y][player->x];
    }
    dungeon[player->y][player->x] = '@';
    *message = "";
    update_visibility();
    return 1;
}

int use_stairs(char direction, int numMonsters, const char** message) {
    if (in_combat) {
        *message = "Cannot use stairs while in combat!";
        return 0;
    }

    if (direction == '>' && terrain[player->y][player->x] == '>') {
        regenerate_dungeon(numMonsters);
        *message = "Descended to a new level!";
        return 1;
    } else if (direction == '<' && terrain[player->y][player->x] == '<') {
        regenerate_dungeon(numMonsters);
        *message = "Ascended to a new level!";
        return 1;
    }
    *message = "Not standing on the correct staircase!";
    return 0;
}

void display_inventory(WINDOW* win, PC* pc, const char** message) {
    werase(win);
    mvwprintw(win, 0, 0, "Inventory (Press any key to exit):");
    for (int i = 0; i < PC::CARRY_SLOTS; i++) {
        if (pc->carry[i]) {
            mvwprintw(win, i + 1, 0, "%d: %s", i, pc->carry[i]->name.c_str());
        } else {
            mvwprintw(win, i + 1, 0, "%d: (empty)", i);
        }
    }
    wrefresh(win);
    getch();
    *message = "Inventory displayed";
}

void display_equipment(WINDOW* win, PC* pc, const char** message) {
    werase(win);
    mvwprintw(win, 0, 0, "Equipment (Press any key to exit):");
    const char* slot_names[] = {
        "Weapon", "Offhand", "Ranged", "Armor", "Helmet", "Cloak",
        "Gloves", "Boots", "Amulet", "Light", "Ring1", "Ring2"
    };
    for (int i = 0; i < PC::EQUIPMENT_SLOTS; i++) {
        if (pc->equipment[i]) {
            mvwprintw(win, i + 1, 0, "%c: %s (%s)", 'a' + i, slot_names[i], pc->equipment[i]->name.c_str());
        } else {
            mvwprintw(win, i + 1, 0, "%c: %s (empty)", 'a' + i, slot_names[i]);
        }
    }
    wrefresh(win);
    getch();
    *message = "Equipment displayed";
}

void wear_item(WINDOW* win, PC* pc, const char** message) {
    werase(win);
    mvwprintw(win, 0, 0, "Select carry slot (0-9, ESC to cancel):");
    for (int i = 0; i < PC::CARRY_SLOTS; i++) {
        if (pc->carry[i]) {
            mvwprintw(win, i + 1, 0, "%d: %s", i, pc->carry[i]->name.c_str());
        }
    }
    wrefresh(win);
    int ch = getch();
    if (ch == 27) {
        *message = "Wear cancelled";
        return;
    }
    if (ch < '0' || ch > '9') {
        *message = "Invalid slot!";
        return;
    }
    int slot = ch - '0';
    if (!pc->carry[slot]) {
        *message = "No item in that slot!";
        return;
    }
    Object* item = pc->carry[slot];
    int equip_slot = -1;
    for (const auto& type : item->types) {
        if (type == "WEAPON") equip_slot = SLOT_WEAPON;
        else if (type == "OFFHAND") equip_slot = SLOT_OFFHAND;
        else if (type == "RANGED") equip_slot = SLOT_RANGED;
        else if (type == "ARMOR") equip_slot = SLOT_ARMOR;
        else if (type == "HELMET") equip_slot = SLOT_HELMET;
        else if (type == "CLOAK") equip_slot = SLOT_CLOAK;
        else if (type == "GLOVES") equip_slot = SLOT_GLOVES;
        else if (type == "BOOTS") equip_slot = SLOT_BOOTS;
        else if (type == "AMULET") equip_slot = SLOT_AMULET;
        else if (type == "LIGHT") equip_slot = SLOT_LIGHT;
        else if (type == "RING") {
            equip_slot = pc->equipment[SLOT_RING1] ? SLOT_RING2 : SLOT_RING1;
        }
    }
    if (equip_slot == -1) {
        *message = "Item cannot be equipped!";
        return;
    }
    if (pc->equipment[equip_slot]) {
        Object* temp = pc->equipment[equip_slot];
        pc->equipment[equip_slot] = item;
        pc->carry[slot] = temp;
    } else {
        pc->equipment[equip_slot] = item;
        pc->carry[slot] = nullptr;
        pc->num_carried--;
    }
    *message = "Item equipped!";
}

void take_off_item(WINDOW* win, PC* pc, const char** message) {
    werase(win);
    mvwprintw(win, 0, 0, "Select equipment slot (a-l, ESC to cancel):");
    const char* slot_names[] = {
        "Weapon", "Offhand", "Ranged", "Armor", "Helmet", "Cloak",
        "Gloves", "Boots", "Amulet", "Light", "Ring1", "Ring2"
    };
    for (int i = 0; i < PC::EQUIPMENT_SLOTS; i++) {
        if (pc->equipment[i]) {
            mvwprintw(win, i + 1, 0, "%c: %s (%s)", 'a' + i, slot_names[i], pc->equipment[i]->name.c_str());
        } else {
            mvwprintw(win, i + 1, 0, "%c: %s (empty)", 'a' + i, slot_names[i]);
        }
    }
    wrefresh(win);
    int ch = getch();
    if (ch == 27) {
        *message = "Take off cancelled";
        return;
    }
    if (ch < 'a' || ch > 'l') {
        *message = "Invalid slot!";
        return;
    }
    int slot = ch - 'a';
    if (!pc->equipment[slot]) {
        *message = "No item in that slot!";
        return;
    }
    if (pc->num_carried >= PC::CARRY_SLOTS) {
        *message = "Inventory full!";
        return;
    }
    for (int i = 0; i < PC::CARRY_SLOTS; i++) {
        if (!pc->carry[i]) {
            pc->carry[i] = pc->equipment[slot];
            pc->equipment[slot] = nullptr;
            pc->num_carried++;
            *message = "Item taken off!";
            return;
        }
    }
}

void drop_item(WINDOW* win, PC* pc, const char** message) {
    werase(win);
    mvwprintw(win, 0, 0, "Select carry slot (0-9, ESC to cancel):");
    for (int i = 0; i < PC::CARRY_SLOTS; i++) {
        if (pc->carry[i]) {
            mvwprintw(win, i + 1, 0, "%d: %s", i, pc->carry[i]->name.c_str());
        }
    }
    wrefresh(win);
    int ch = getch();
    if (ch == 27) {
        *message = "Drop cancelled";
        return;
    }
    if (ch < '0' || ch > '9') {
        *message = "Invalid slot!";
        return;
    }
    int slot = ch - '0';
    if (!pc->carry[slot]) {
        *message = "No item in that slot!";
        return;
    }
    Object* item = pc->carry[slot];
    item->x = pc->x;
    item->y = pc->y;
    objectAt[item->y][item->x] = item;
    for (int i = 0; i < num_objects; i++) {
        if (!objects[i]) {
            objects[i] = item;
            break;
        }
    }
    pc->carry[slot] = nullptr;
    pc->num_carried--;
    *message = "Item dropped!";
}

void expunge_item(WINDOW* win, PC* pc, const char** message) {
    werase(win);
    mvwprintw(win, 0, 0, "Select carry slot (0-9, ESC to cancel):");
    for (int i = 0; i < PC::CARRY_SLOTS; i++) {
        if (pc->carry[i]) {
            mvwprintw(win, i + 1, 0, "%d: %s", i, pc->carry[i]->name.c_str());
        }
    }
    wrefresh(win);
    int ch = getch();
    if (ch == 27) {
        *message = "Expunge cancelled";
        return;
    }
    if (ch < '0' || ch > '9') {
        *message = "Invalid slot!";
        return;
    }
    int slot = ch - '0';
    if (!pc->carry[slot]) {
        *message = "No item in that slot!";
        return;
    }
    delete pc->carry[slot];
    pc->carry[slot] = nullptr;
    pc->num_carried--;
    *message = "Item expunged!";
}

void inspect_item(WINDOW* win, PC* pc, const char** message) {
    werase(win);
    mvwprintw(win, 0, 0, "Select carry slot (0-9, ESC to cancel):");
    for (int i = 0; i < PC::CARRY_SLOTS; i++) {
        if (pc->carry[i]) {
            mvwprintw(win, i + 1, 0, "%d: %s", i, pc->carry[i]->name.c_str());
        }
    }
    wrefresh(win);
    int ch = getch();
    if (ch == 27) {
        *message = "Inspect cancelled";
        return;
    }
    if (ch < '0' || ch > '9') {
        *message = "Invalid slot!";
        return;
    }
    int slot = ch - '0';
    if (!pc->carry[slot]) {
        *message = "No item in that slot!";
        return;
    }
    Object* item = pc->carry[slot];
    werase(win);
    mvwprintw(win, 0, 0, "Item Information (Press any key to exit):");
    mvwprintw(win, 1, 0, "Name: %s", item->name.c_str());
    mvwprintw(win, 2, 0, "Type: %s", item->types[0].c_str());
    mvwprintw(win, 3, 0, "Damage: %s", item->damage.toString().c_str());
    mvwprintw(win, 4, 0, "Speed: %d", item->speed);
    int line = 5;
    for (const auto& desc : objectDescs) {
        if (desc.name == item->name) {
            for (const auto& line_text : desc.description) {
                if (line < 23) {
                    mvwprintw(win, line++, 2, "%s", line_text.c_str());
                }
            }
            break;
        }
    }
    wrefresh(win);
    getch();
    *message = "Item inspected";
}