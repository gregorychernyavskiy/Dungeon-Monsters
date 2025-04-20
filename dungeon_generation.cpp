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

std::vector<MonsterDescription> monsterDescs;
std::vector<ObjectDescription> objectDescs;

Object::Object(int x_, int y_) : x(x_), y(y_), hit(0), dodge(0), defense(0),
    weight(0), speed(0), attribute(0), value(0), is_artifact(false) {}

Object::~Object() {}

Character::Character(int x_, int y_) : x(x_), y(y_), speed(10), alive(1),
    last_seen_x(-1), last_seen_y(-1), hitpoints(10) {}

PC::PC(int x_, int y_) : Character(x_, y_) {
    symbol = '@';
    color = "WHITE";
    hitpoints = 100;
    damage = Dice(0, 1, 4);
    total_speed = speed;
    for (int i = 0; i < 12; i++) equipment[i] = nullptr;
    for (int i = 0; i < 10; i++) carry[i] = nullptr;
}

PC::~PC() {
    for (int i = 0; i < 12; i++) delete equipment[i];
    for (int i = 0; i < 10; i++) delete carry[i];
}

bool PC::pickup_object(Object* obj) {
    for (int i = 0; i < 10; i++) {
        if (!carry[i]) {
            carry[i] = obj;
            objectAt[obj->y][obj->x] = nullptr;
            terrain[obj->y][obj->x] = dungeon[obj->y][obj->x];
            return true;
        }
    }
    return false;
}

void PC::recalculate_stats() {
    total_speed = speed;
    for (int i = 0; i < 12; i++) {
        if (equipment[i]) {
            total_speed += equipment[i]->speed;
        }
    }
}

NPC::NPC(int x_, int y_) : Character(x_, y_), intelligent(0),
    tunneling(0), telepathic(0), erratic(0),
    pass_wall(0), pickup(0), destroy(0), is_unique(false) {
    speed = rand() % 16 + 5;
}

void PC::move() {}

void NPC::move() {
    if (!alive) return;
    static bool combat_triggered = false; // Limit to one combat per turn
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
        if (next_x == player->x && next_y == player->y && !combat_triggered) {
            const char* message;
            combat_triggered = true;
            if (name == "SpongeBob SquarePants") {
                forced_combat(this, player, stdscr, &message);
            } else {
                combat(this, player, &message);
            }
            return;
        }
        if (monsterAt[next_y][next_x]) {
            NPC* other = monsterAt[next_y][next_x];
            int displace_x = -1, displace_y = -1;
            for (int i = 0; i < 8; i++) {
                int nx = next_x + dx[i];
                int ny = next_y + dy[i];
                if (nx >= 0 && nx < WIDTH && ny >= 0 && ny < HEIGHT &&
                    !monsterAt[ny][nx] && (tunneling || pass_wall || hardness[ny][nx] == 0)) {
                    displace_x = nx;
                    displace_y = ny;
                    break;
                }
            }
            if (displace_x != -1 && displace_y != -1) {
                monsterAt[other->y][other->x] = nullptr;
                other->x = displace_x;
                other->y = displace_y;
                monsterAt[other->y][other->x] = other;
            } else {
                monsterAt[curr_y][curr_x] = other;
                other->x = curr_x;
                other->y = curr_y;
                monsterAt[next_y][next_x] = this;
                x = next_x;
                y = next_y;
                return;
            }
        }
        bool was_rock = hardness[next_y][next_x] > 0;
        if (was_rock && tunneling && !pass_wall) {
            hardness[next_y][next_x] = 0;
            dungeon[next_y][next_x] = ' ';
            terrain[next_y][next_x] = ' ';
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
                // Optional: Implement inventory later
            }
        }
        monsterAt[curr_y][curr_x] = nullptr;
        if ((name == "SpongeBob SquarePants" || name == "Slimer" || name == "Casper the Friendly Ghost") && was_rock) {
            dungeon[curr_y][curr_x] = ' ';
            terrain[curr_y][curr_x] = ' ';
        } else {
            dungeon[curr_y][curr_x] = terrain[curr_y][curr_x] ? terrain[curr_y][curr_x] : ' ';
        }
        x = next_x;
        y = next_y;
        monsterAt[next_y][next_x] = this;
    }
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

int gameOver(NPC** culprit, bool* boss_killed) {
    if (player->hitpoints <= 0) {
        *culprit = nullptr;
        *boss_killed = false;
        return 1;
    }
    for (int i = 0; i < num_monsters; i++) {
        if (monsters[i] && monsters[i]->name == "SpongeBob SquarePants" && !monsters[i]->alive) {
            *culprit = nullptr;
            *boss_killed = true;
            return 1;
        }
    }
    *culprit = nullptr;
    *boss_killed = false;
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

    refresh();
}

int getColorIndex(const std::string& color) {
    std::string trimmed_color = color;
    trimmed_color.erase(0, trimmed_color.find_first_not_of(" \t"));
    trimmed_color.erase(trimmed_color.find_last_not_of(" \t") + 1);

    std::string upper_color = trimmed_color;
    for (char& c : upper_color) {
        c = std::toupper(c);
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
    werase(win);
    mvwprintw(win, 0, 0, "%s", message ? message : "");

    for (int y = 0; y < HEIGHT; y++) {
        for (int x = 0; x < WIDTH; x++) {
            if (fog_enabled && !visible[y][x] && !remembered[y][x]) {
                mvwprintw(win, y + 1, x, " ");
            } else if (x == player->x && y == player->y) {
                int color = getColorIndex(player->color);
                wattron(win, COLOR_PAIR(color));
                mvwprintw(win, y + 1, x, "@");
                wattroff(win, COLOR_PAIR(color));
            } else if (monsterAt[y][x] && (!fog_enabled || visible[y][x])) {
                int color = getColorIndex(monsterAt[y][x]->color);
                wattron(win, COLOR_PAIR(color));
                mvwprintw(win, y + 1, x, "%c", monsterAt[y][x]->symbol);
                wattroff(win, COLOR_PAIR(color));
            } else if (objectAt[y][x] && (!fog_enabled || visible[y][x])) {
                int color = getColorIndex(objectAt[y][x]->color);
                wattron(win, COLOR_PAIR(color));
                mvwprintw(win, y + 1, x, "%c", objectAt[y][x]->symbol);
                wattroff(win, COLOR_PAIR(color));
            } else {
                char display = (fog_enabled && remembered[y][x] && !visible[y][x]) ? remembered[y][x] : dungeon[y][x];
                mvwprintw(win, y + 1, x, "%c", display);
            }
        }
    }
    mvwprintw(win, 22, 0, "HP: %d Speed: %d", player->hitpoints, player->total_speed);
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
                mvwprintw(win, i - start + 1, 0, "%c (%s), %d %s and %d %s, HP: %d", 
                          monsters[i]->symbol, monsters[i]->name.c_str(), abs(dy), ns, abs(dx), ew, monsters[i]->hitpoints);
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
                for (/auto& desc : monsterDescs) {
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
        if (monsterAt[new_y][new_x]->name == "SpongeBob SquarePants") {
            forced_combat(player, monsterAt[new_y][new_x], stdscr, message);
        } else {
            combat(player, monsterAt[new_y][new_x], message);
        }
        return 1;
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
    if (objectAt[new_y][new_x]) {
        if (player->pickup_object(objectAt[new_y][new_x])) {
            *message = "Picked up an item!";
        } else {
            *message = "Inventory full!";
        }
    } else {
        *message = "";
    }
    update_visibility();
    return 1;
}

int use_stairs(char direction, int numMonsters, const char** message) {
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

int combat(Character* attacker, Character* defender, const char** message) {
    std::random_device rd;
    std::mt19937 gen(rd());
    int damage = 0;
    char buf[80];

    if (dynamic_cast<PC*>(attacker)) {
        if (!player->equipment[0]) {
            damage += player->damage.base;
            for (int i = 0; i < player->damage.dice; i++) {
                std::uniform_int_distribution<> dis(1, player->damage.sides);
                damage += dis(gen);
            }
        }
        for (int i = 0; i < 12; i++) {
            if (player->equipment[i]) {
                damage += player->equipment[i]->damage.base;
                for (int j = 0; j < player->equipment[i]->damage.dice; j++) {
                    std::uniform_int_distribution<> dis(1, player->equipment[i]->damage.sides);
                    damage += dis(gen);
                }
            }
        }
    } else {
        NPC* npc = dynamic_cast<NPC*>(attacker);
        damage += npc->damage.base;
        for (int i = 0; i < npc->damage.dice; i++) {
            std::uniform_int_distribution<> dis(1, npc->damage.sides);
            damage += dis(gen);
        }
    }

    defender->hitpoints -= damage;
    snprintf(buf, sizeof(buf), "%s deals %d damage to %s (HP: %d)", 
             attacker->name.c_str(), damage, defender->name.c_str(), defender->hitpoints);
    *message = buf;

    if (defender->hitpoints <= 0) {
        defender->alive = 0;
        if (dynamic_cast<PC*>(defender)) {
            *message = "You died!";
        } else {
            NPC* npc = dynamic_cast<NPC*>(defender);
            monsterAt[npc->y][npc->x] = nullptr;
            snprintf(buf, sizeof(buf), "%s killed %s!", attacker->name.c_str(), npc->name.c_str());
            *message = buf;
        }
    }
    return 1;
}

int forced_combat(Character* attacker, Character* defender, WINDOW* win, const char** message) {
    char buf[80];
    bool pc_turn = dynamic_cast<PC*>(attacker) != nullptr;
    snprintf(buf, sizeof(buf), "%s is predicting to kill %s!", 
             attacker->name.c_str(), defender->name.c_str());
    *message = buf;
    draw_dungeon(win, *message);
    getch();

    while (attacker->alive && defender->alive) {
        if (pc_turn) {
            defender->hitpoints -= 5;
            snprintf(buf, sizeof(buf), "%s deals 5 damage to %s (HP: %d)", 
                     attacker->name.c_str(), defender->name.c_str(), defender->hitpoints);
        } else {
            attacker->hitpoints -= 5;
            snprintf(buf, sizeof(buf), "%s deals 5 damage to %s (HP: %d)", 
                     defender->name.c_str(), attacker->name.c_str(), attacker->hitpoints);
        }
        *message = buf;
        draw_dungeon(win, *message);
        getch();

        if (defender->hitpoints <= 0) {
            defender->alive = 0;
            if (dynamic_cast<PC*>(defender)) {
                *message = "You died!";
            } else {
                NPC* npc = dynamic_cast<NPC*>(defender);
                monsterAt[npc->y][npc->x] = nullptr;
                snprintf(buf, sizeof(buf), "%s killed %s!", attacker->name.c_str(), npc->name.c_str());
                *message = buf;
            }
            draw_dungeon(win, *message);
            getch();
            break;
        }
        if (attacker->hitpoints <= 0) {
            attacker->alive = 0;
            if (dynamic_cast<PC*>(attacker)) {
                *message = "You died!";
            } else {
                NPC* npc = dynamic_cast<NPC*>(attacker);
                monsterAt[npc->y][npc->x] = nullptr;
                snprintf(buf, sizeof(buf), "%s killed %s!", defender->name.c_str(), npc->name.c_str());
                *message = buf;
            }
            draw_dungeon(win, *message);
            getch();
            break;
        }
        pc_turn = !pc_turn;
    }
    return 1;
}

void inspect_monster(WINDOW* win, int target_x, int target_y) {
    if (monsterAt[target_y][target_x]) {
        NPC* monster = monsterAt[target_y][target_x];
        for (const auto& desc : monsterDescs) {
            if (desc.name == monster->name) {
                werase(win);
                mvwprintw(win, 0, 0, "Monster: %s", desc.name.c_str());
                for (size_t i = 0; i < desc.description.size(); i++) {
                    mvwprintw(win, i + 1, 0, "%s", desc.description[i].c_str());
                }
                mvwprintw(win, desc.description.size() + 1, 0, "Symbol: %c", desc.symbol);
                mvwprintw(win, desc.description.size() + 2, 0, "HP: %d", monster->hitpoints);
                mvwprintw(win, desc.description.size() + 3, 0, "Damage: %s", desc.damage.toString().c_str());
                mvwprintw(win, desc.description.size() + 4, 0, "Speed: %s", desc.speed.toString().c_str());
                wrefresh(win);
                getch();
                break;
            }
        }
    } else {
        werase(win);
        mvwprintw(win, 0, 0, "No monster at this location!");
        wrefresh(win);
        getch();
    }
}