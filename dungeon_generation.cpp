#include "dungeon_generation.h"
#include <unistd.h>
#include <ctype.h>
#include <ncurses.h>

char dungeon[HEIGHT][WIDTH];
unsigned char hardness[HEIGHT][WIDTH];
struct Room rooms[MAX_ROOMS];
int distance_non_tunnel[HEIGHT][WIDTH];
int distance_tunnel[HEIGHT][WIDTH];
NPC* monsterAt[HEIGHT][WIDTH] = {nullptr};

NPC** monsters = nullptr;
int num_monsters = 0;

PC* player = nullptr;
int num_rooms = 0;
int randRoomNum = 0;
int upStairsCount = 0;
int downStairsCount = 0;
int player_room_index = -1;

char* dungeonFile = nullptr;
struct Stairs upStairs[MAX_ROOMS];
struct Stairs downStairs[MAX_ROOMS];

int fog_enabled = 1; // Enabled by default
char visible[HEIGHT][WIDTH] = {0};
char terrain[HEIGHT][WIDTH] = {0};
char remembered[HEIGHT][WIDTH] = {0};

void PC::move() {
    // Actual movement handled in move_player(), this is a placeholder
}

void NPC::move() {
    if (!alive) return;
    int dist[HEIGHT][WIDTH];
    if (tunneling) dijkstraTunneling(dist);
    else dijkstraNonTunneling(dist);
    int curr_x = x, curr_y = y;
    int min_dist = dist[curr_y][curr_x];
    int next_x = curr_x, next_y = curr_y;

    int dx[] = {-1, 0, 1, -1, 1, -1, 0, 1};
    int dy[] = {-1, -1, -1, 0, 0, 1, 1, 1};

    for (int i = 0; i < 8; i++) {
        int nx = curr_x + dx[i];
        int ny = curr_y + dy[i];
        if (nx >= 0 && nx < WIDTH && ny >= 0 && ny < HEIGHT) {
            if (dist[ny][nx] < min_dist && (tunneling || hardness[ny][nx] == 0) && !monsterAt[ny][nx]) {
                min_dist = dist[ny][nx];
                next_x = nx;
                next_y = ny;
            }
        }
    }
    if (next_x != curr_x || next_y != curr_y) {
        monsterAt[curr_y][curr_x] = nullptr;
        x = next_x;
        y = next_y;
        monsterAt[next_y][next_x] = this;
    }
}

void printDungeon() {
    for (int y = 0; y < HEIGHT; y++) {
        for (int x = 0; x < WIDTH; x++) {
            if (monsterAt[y][x]) {
                int personality = monsterAt[y][x]->intelligent + (monsterAt[y][x]->telepathic << 1) +
                                  (monsterAt[y][x]->tunneling << 2) + (monsterAt[y][x]->erratic << 3);
                char symbol = personality < 10 ? '0' + personality : 'A' + (personality - 10);
                printf("%c", symbol);
            } else if (x == player->x && y == player->y) {
                printf("@");
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

NPC* generateMonsterByType(char c, int x, int y) {
    NPC* monster = new NPC(x, y);
    c = tolower(c);
    int num = (c >= '0' && c <= '9') ? (c - '0') : (c >= 'a' && c <= 'f') ? (c - 'a' + 10) : -1;
    if (num == -1) {
        delete monster;
        printf("Error: Invalid hex character '%c'\n", c);
        return nullptr;
    }
    monster->intelligent = num & 1;
    monster->telepathic = (num >> 1) & 1;
    monster->tunneling = (num >> 2) & 1;
    monster->erratic = (num >> 3) & 1;
    return monster;
}

NPC* generateMonster(int x, int y) {
    return new NPC(x, y);
}

int spawnMonsterByType(char monType) {
    NPC** temp = (NPC**)realloc(monsters, (num_monsters + 1) * sizeof(NPC*));
    if (!temp) {
        fprintf(stderr, "Error: Failed to allocate memory for monsters\n");
        return 1;
    }
    monsters = temp;

    int attempts = 100;
    for (int j = 0; j < attempts; j++) {
        int x = rand() % WIDTH;
        int y = rand() % HEIGHT;
        struct Room playerRoom = rooms[player_room_index];
        bool in_player_room = (x >= playerRoom.x && x < playerRoom.x + playerRoom.width &&
                               y >= playerRoom.y && y < playerRoom.y + playerRoom.height);
        if (dungeon[y][x] != '.' || (x == player->x && y == player->y) || monsterAt[y][x] || in_player_room) {
            continue;
        }
        monsters[num_monsters] = generateMonsterByType(monType, x, y);
        if (!monsters[num_monsters]) return 1;
        monsterAt[y][x] = monsters[num_monsters];
        num_monsters++;
        return 0;
    }
    fprintf(stderr, "Error: Failed to spawn monster\n");
    return 1;
}

int spawnMonsters(int numMonsters) {
    if (monsters) {
        for (int i = 0; i < num_monsters; i++) {
            if (monsters[i] && monsterAt[monsters[i]->y][monsters[i]->x]) {
                monsterAt[monsters[i]->y][monsters[i]->x] = nullptr;
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

    int tunneling_count = numMonsters / 2;
    int nontunneling_count = numMonsters - tunneling_count;

    for (int i = 0; i < tunneling_count; i++) {
        int placed = 0;
        for (int j = 0; j < 100; j++) {
            int x = rand() % WIDTH;
            int y = rand() % HEIGHT;
            struct Room playerRoom = rooms[player_room_index];
            bool in_player_room = (x >= playerRoom.x && x < playerRoom.x + playerRoom.width &&
                                   y >= playerRoom.y && y < playerRoom.y + playerRoom.height);
            if (dungeon[y][x] == '.' && !(x == player->x && y == player->y) && !monsterAt[y][x] && !in_player_room) {
                NPC** temp = (NPC**)realloc(monsters, (num_monsters + 1) * sizeof(NPC*));
                if (!temp) continue;
                monsters = temp;
                monsters[num_monsters] = generateMonster(x, y);
                if (monsters[num_monsters]) {
                    monsters[num_monsters]->tunneling = 1;
                    monsterAt[y][x] = monsters[num_monsters];
                    num_monsters++;
                    placed = 1;
                }
                break;
            }
        }
        if (!placed) {
            fprintf(stderr, "Error: Failed to place tunneling monster\n");
            return 1;
        }
    }

    for (int i = 0; i < nontunneling_count; i++) {
        int placed = 0;
        for (int j = 0; j < 100; j++) {
            int x = rand() % WIDTH;
            int y = rand() % HEIGHT;
            struct Room playerRoom = rooms[player_room_index];
            bool in_player_room = (x >= playerRoom.x && x < playerRoom.x + playerRoom.width &&
                                   y >= playerRoom.y && y < playerRoom.y + playerRoom.height);
            if (dungeon[y][x] == '.' && !(x == player->x && y == player->y) && !monsterAt[y][x] && !in_player_room) {
                NPC** temp = (NPC**)realloc(monsters, (num_monsters + 1) * sizeof(NPC*));
                if (!temp) continue;
                monsters = temp;
                monsters[num_monsters] = generateMonster(x, y);
                if (monsters[num_monsters]) {
                    monsters[num_monsters]->tunneling = 0;
                    monsterAt[y][x] = monsters[num_monsters];
                    num_monsters++;
                    placed = 1;
                }
                break;
            }
        }
        if (!placed) {
            fprintf(stderr, "Error: Failed to place non-tunneling monster\n");
            return 1;
        }
    }
    return 0;
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
    raw();
    noecho();
    keypad(stdscr, TRUE);
    curs_set(0);
}

void update_visibility() {
    const int radius = 3; // Light radius per assignment
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
            } else if (monsterAt[y][x] && (!fog_enabled || visible[y][x])) {
                int personality = monsterAt[y][x]->intelligent + (monsterAt[y][x]->telepathic << 1) +
                                  (monsterAt[y][x]->tunneling << 2) + (monsterAt[y][x]->erratic << 3);
                char symbol = personality < 10 ? '0' + personality : 'A' + (personality - 10);
                mvwprintw(win, y + 1, x, "%c", symbol);
            } else if (x == player->x && y == player->y) {
                mvwprintw(win, y + 1, x, "@");
            } else {
                char display = (fog_enabled && remembered[y][x] && !visible[y][x]) ? remembered[y][x] : dungeon[y][x];
                mvwprintw(win, y + 1, x, "%c", display);
            }
        }
    }
    mvwprintw(win, 22, 0, "Status Line 1");
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
                int personality = monsters[i]->intelligent + (monsters[i]->telepathic << 1) +
                                  (monsters[i]->tunneling << 2) + (monsters[i]->erratic << 3);
                char symbol = personality < 10 ? '0' + personality : 'A' + (personality - 10);
                const char* ns = dy < 0 ? "north" : "south";
                const char* ew = dx < 0 ? "west" : "east";
                mvwprintw(win, i - start + 1, 0, "%c, %d %s and %d %s", symbol, abs(dy), ns, abs(dx), ew);
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
            delete monsters[i];
        }
    }
    free(monsters);
    monsters = nullptr;
    num_monsters = 0;

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
        *message = "A monster blocks your path!";
        return 0;
    }
    dungeon[player->y][player->x] = terrain[player->y][player->x];
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