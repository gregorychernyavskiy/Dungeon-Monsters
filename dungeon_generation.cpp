#include "dungeon_generation.h"
#include <unistd.h>
#include <ctype.h>
#include <ncurses.h>
#include <random>
#include <ctime>
#include <cstring>
#include <cstdlib>
#include <map>

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

int current_level = 1; // Start at level 1
std::map<int, std::vector<Object*>> level_objects; // Store objects per level
std::priority_queue<Event, std::vector<Event>, std::greater<Event>> event_queue;
int64_t game_turn = 0;

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
    speed = 10; // Fixed speed as per assignment
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
    // Speed is set by MonsterDescription::createNPC
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

void PC::calculateStats(int& total_speed, Dice& total_damage, int& total_defense, int& total_hit, int& total_dodge) {
    total_speed = speed; // Base speed: 10
    total_damage = damage; // Base damage: 0+1d4
    total_defense = 0; // Base defense: 0
    total_hit = 0; // Base hit: 0
    total_dodge = 0; // Base dodge: 0

    // Add equipment bonuses
    for (int i = 0; i < EQUIPMENT_SLOTS; i++) {
        if (equipment[i]) {
            total_speed += equipment[i]->speed;
            total_defense += equipment[i]->defense;
            total_hit += equipment[i]->hit;
            total_dodge += equipment[i]->dodge;
            if (i == SLOT_WEAPON) {
                total_damage = Dice(0, 0, 0); // Reset damage if weapon is equipped
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

void schedule_event(Character* character) {
    int64_t delay = 1000 / character->speed; // floor(1000/speed) using integer division
    int64_t next_time = game_turn + delay;
    event_queue.emplace(next_time, character, Event::MOVE);
}

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
        int total_defense, total_hit, total_dodge;
        player->calculateStats(total_speed, total_damage, total_defense, total_hit, total_dodge);
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
            schedule_event(player); // Reschedule player event
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
            schedule_event(player); // Reschedule player event
            schedule_event(monster); // Reschedule monster event
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
    if (player) {
        // Preserve inventory and equipment
        PC* old_player = player;
        player = new PC(px, py);
        player->num_carried = old_player->num_carried;
        player->hitpoints = old_player->hitpoints;
        for (int i = 0; i < PC::CARRY_SLOTS; i++) {
            player->carry[i] = old_player->carry[i];
            old_player->carry[i] = nullptr; // Prevent deletion
        }
        for (int i = 0; i < PC::EQUIPMENT_SLOTS; i++) {
            player->equipment[i] = old_player->equipment[i];
            old_player->equipment[i] = nullptr; // Prevent deletion
        }
        delete old_player;
    } else {
        player = new PC(px, py);
    }
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
    // Clean up existing monsters
    if (monsters) {
        for (int i = 0; i < num_monsters; ++i) { // Use num_monsters (current count)
            if (i < num_monsters && monsters[i]) { // Ensure we don't access out of bounds
                if (monsterAt[monsters[i]->y][monsters[i]->x]) {
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
    }

    // Reset num_monsters before spawning
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

    // Spawn exactly numMonsters monsters
    while (num_monsters < numMonsters) {
        NPC* npc = nullptr;
        int attempts = 100;
        while (attempts--) {
            int idx = desc_dis(gen);
            MonsterDescription& desc = monsterDescs[idx];
            if (desc.is_unique && desc.is_alive) continue; // Skip if unique and already spawned
            if (desc.rarity <= dis(gen)) continue; // Skip if rarity check fails

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
            if (!temp) {
                fprintf(stderr, "Error: Failed to reallocate memory for monsters\n");
                delete npc;
                continue;
            }
            monsters = temp;
            monsters[num_monsters] = npc;
            monsterAt[npc->y][npc->x] = npc;
            num_monsters++;
            schedule_event(npc); // Schedule initial event for the monster
        } else {
            // If we couldn't place a monster after attempts, break to avoid infinite loop
            break;
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
    // Store current level's objects before cleanup
    std::vector<Object*> current_objects;
    for (int i = 0; i < num_objects; ++i) {
        if (objects[i]) {
            current_objects.push_back(objects[i]);
        }
    }
    level_objects[current_level] = current_objects;

    // Clear objectAt array
    for (int i = 0; i < num_objects; ++i) {
        if (objects[i]) {
            objectAt[objects[i]->y][objects[i]->x] = nullptr;
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

    // Clear any existing events
    while (!event_queue.empty()) event_queue.pop();

    // Schedule initial events for PC and NPCs
    schedule_event(player);
    for (int i = 0; i < num_monsters; i++) {
        if (monsters[i] && monsters[i]->alive) {
            schedule_event(monsters[i]);
        }
    }

    int monsters_alive = num_monsters;
    while (monsters_alive > 0 && player->alive) {
        // Get the next event
        Event event = event_queue.top();
        event_queue.pop();

        // Update game turn to the event's time
        game_turn = event.time;

        // Process the event
        if (event.character->alive) {
            event.character->move();
            schedule_event(event.character); // Schedule next move
        }

        // Check for game over
        if (!player->alive) {
            printf("You were killed! Game over!\n");
            break;
        }

        // Count living monsters
        monsters_alive = 0;
        for (int i = 0; i < num_monsters; i++) {
            if (monsters[i] && monsters[i]->alive) {
                monsters_alive++;
            }
        }

        // Redraw dungeon
        printDungeon();
        usleep(250000); // Pause for 250ms as per assignment
    }

    // Print final state
    printf("\nFinal state:\n");
    printDungeon();
    if (monsters_alive == 0 && player->alive) {
        printf("You win!\n");
    } else {
        printf("Game over!\n");
    }
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
    int total_defense, total_hit, total_dodge;
    player->calculateStats(total_speed, total_damage, total_defense, total_hit, total_dodge);
    mvwprintw(win, 22, 0, "HP:%d SPD:%d DEF:%d HIT:%d DGE:%d", 
              player->hitpoints, total_speed, total_defense, total_hit, total_dodge);
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
    // Save current level's objects
    cleanupObjects();

    // Clear monsters
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

    // Clear event queue
    while (!event_queue.empty()) event_queue.pop();

    // Update level
    current_level++;

    // Clear player's previous position
    if (player->x >= 0 && player->y >= 0 && player->x < WIDTH && player->y < HEIGHT) {
        dungeon[player->y][player->x] = terrain[player->y][player->x] == 0 ? '.' : terrain[player->y][player->x];
    }

    // Generate new dungeon
    emptyDungeon();
    createRooms();
    connectRooms();
    placeStairs();

    // Update terrain
    for (int y = 0; y < HEIGHT; y++) {
        for (int x = 0; x < WIDTH; x++) {
            terrain[y][x] = dungeon[y][x];
        }
    }

    // Place player (preserves inventory and equipment)
    placePlayer();

    initializeHardness();
    spawnMonsters(numMonsters);

    // Schedule events for new level
    schedule_event(player);
    for (int i = 0; i < num_monsters; i++) {
        if (monsters[i] && monsters[i]->alive) {
            schedule_event(monsters[i]);
        }
    }

    // Restore or place new objects
    if (level_objects.find(current_level) != level_objects.end()) {
        objects = (Object**)malloc(level_objects[current_level].size() * sizeof(Object*));
        num_objects = level_objects[current_level].size();
        for (size_t i = 0; i < level_objects[current_level].size(); ++i) {
            objects[i] = level_objects[current_level][i];
            objectAt[objects[i]->y][objects[i]->x] = objects[i];
        }
    } else {
        placeObjects(10);
    }

    memset(visible, 0, sizeof(visible));
    memset(remembered, 0, sizeof(remembered));
    update_visibility();
}

void display_help(WINDOW* win, const char** message) {
    werase(win);
    mvwprintw(win, 0, 0, "Help Menu (Press any key to exit):");
    mvwprintw(win, 1, 0, "Movement:");
    mvwprintw(win, 2, 0, "  7/y, 8/k, 9/u: Up-Left, Up, Up-Right");
    mvwprintw(win, 3, 0, "  4/h, 6/l: Left, Right");
    mvwprintw(win, 4, 0, "  1/b, 2/j, 3/n: Down-Left, Down, Down-Right");
    mvwprintw(win, 5, 0, "Combat:");
    mvwprintw(win, 6, 0, "  a: Attack (when in combat)");
    mvwprintw(win, 7, 0, "  f: Flee (when in combat, 30%% chance)");
    mvwprintw(win, 8, 0, "Inventory & Equipment:");
    mvwprintw(win, 9, 0, "  i: View inventory");
    mvwprintw(win, 10, 0, "  w: Wear item from inventory");
    mvwprintw(win, 11, 0, "  d: Drop item from inventory");
    mvwprintw(win, 12, 0, "  x: Expunge item from inventory");
    mvwprintw(win, 13, 0, "  I: Inspect item in inventory");
    mvwprintw(win, 14, 0, "  e: View equipment");
    mvwprintw(win, 15, 0, "  t: Take off equipped item");
    mvwprintw(win, 16, 0, "Other Actions:");
    mvwprintw(win, 17, 0, "  5/space/.: Rest");
    mvwprintw(win, 18, 0, "  m: View monster list");
    mvwprintw(win, 19, 0, "  f: Toggle fog of war");
    mvwprintw(win, 20, 0, "  g: Teleport (g to confirm, r for random, ESC to cancel)");
    mvwprintw(win, 21, 0, "  L: Look mode (t to inspect monster, ESC to cancel)");
    mvwprintw(win, 22, 0, "  >/ <: Use stairs");
    mvwprintw(win, 23, 0, "  s: View stats  q/Q: Quit");
    wrefresh(win);
    getch();
    *message = "Help displayed";
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
            Object* item = pc->equipment[i];
            std::string stats;
            bool has_stats = false;

            if (item->damage.base != 0 || item->damage.dice != 0) {
                stats += (has_stats ? ", " : "") + std::string("+") + item->damage.toString() + " damage";
                has_stats = true;
            }
            if (item->speed != 0) {
                stats += (has_stats ? ", " : "") + std::string(item->speed > 0 ? "+" : "") + std::to_string(item->speed) + " speed";
                has_stats = true;
            }
            if (item->defense != 0) {
                stats += (has_stats ? ", " : "") + std::string(item->defense > 0 ? "+" : "") + std::to_string(item->defense) + " defense";
                has_stats = true;
            }
            if (item->hit != 0) {
                stats += (has_stats ? ", " : "") + std::string(item->hit > 0 ? "+" : "") + std::to_string(item->hit) + " hit";
                has_stats = true;
            }
            if (item->dodge != 0) {
                stats += (has_stats ? ", " : "") + std::string(item->dodge > 0 ? "+" : "") + std::to_string(item->dodge) + " dodge";
                has_stats = true;
            }

            if (has_stats) {
                mvwprintw(win, i + 1, 0, "%c: %s (%s) (%s)", 'a' + i, slot_names[i], item->name.c_str(), stats.c_str());
            } else {
                mvwprintw(win, i + 1, 0, "%c: %s (%s) (No boosts)", 'a' + i, slot_names[i], item->name.c_str());
            }
        } else {
            mvwprintw(win, i + 1, 0, "%c: %s (empty)", 'a' + i, slot_names[i]);
        }
    }
    wrefresh(win);
    getch();
    *message = "Equipment displayed";
}

void display_stats(WINDOW* win, PC* pc, const char** message) {
    werase(win);
    mvwprintw(win, 0, 0, "Player Stats (Press any key to exit):");

    int total_speed;
    Dice total_damage;
    int total_defense, total_hit, total_dodge;
    pc->calculateStats(total_speed, total_damage, total_defense, total_hit, total_dodge);

    // Calculate a sample damage roll for display
    std::random_device rd;
    std::mt19937 gen(rd());
    int sample_damage = total_damage.base;
    for (int i = 0; i < total_damage.dice; i++) {
        std::uniform_int_distribution<> dis(1, total_damage.sides);
        sample_damage += dis(gen);
    }

    mvwprintw(win, 1, 0, "Hitpoints: %d", pc->hitpoints);
    mvwprintw(win, 2, 0, "Speed: %d", total_speed);
    if (total_damage.base == 0 && total_damage.dice == 0) {
        mvwprintw(win, 3, 0, "Damage: No damage");
    } else {
        mvwprintw(win, 3, 0, "Damage: %s (e.g., %d)", total_damage.toString().c_str(), sample_damage);
    }
    mvwprintw(win, 4, 0, "Defense: %d", total_defense);
    mvwprintw(win, 5, 0, "Hit: %d", total_hit);
    mvwprintw(win, 6, 0, "Dodge: %d", total_dodge);
    mvwprintw(win, 7, 0, "Position: (%d, %d)", pc->x, pc->y);

    wrefresh(win);
    getch();
    *message = "Stats displayed";
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
            Object* item = pc->equipment[i];
            std::string stats;
            bool has_stats = false;

            if (item->damage.base != 0 || item->damage.dice != 0) {
                stats += (has_stats ? ", " : "") + std::string("+") + item->damage.toString() + " damage";
                has_stats = true;
            }
            if (item->speed != 0) {
                stats += (has_stats ? ", " : "") + std::string(item->speed > 0 ? "+" : "") + std::to_string(item->speed) + " speed";
                has_stats = true;
            }
            if (item->defense != 0) {
                stats += (has_stats ? ", " : "") + std::string(item->defense > 0 ? "+" : "") + std::to_string(item->defense) + " defense";
                has_stats = true;
            }
            if (item->hit != 0) {
                stats += (has_stats ? ", " : "") + std::string(item->hit > 0 ? "+" : "") + std::to_string(item->hit) + " hit";
                has_stats = true;
            }
            if (item->dodge != 0) {
                stats += (has_stats ? ", " : "") + std::string(item->dodge > 0 ? "+" : "") + std::to_string(item->dodge) + " dodge";
                has_stats = true;
            }

            if (has_stats) {
                mvwprintw(win, i + 1, 0, "%c: %s (%s) (%s)", 'a' + i, slot_names[i], item->name.c_str(), stats.c_str());
            } else {
                mvwprintw(win, i + 1, 0, "%c: %s (%s) (No boosts)", 'a' + i, slot_names[i], item->name.c_str());
            }
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