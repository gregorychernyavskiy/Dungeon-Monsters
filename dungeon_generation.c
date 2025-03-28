#include "dungeon_generation.h"
#include "minheap.h"
#include <unistd.h>
#include <ctype.h>
#include <ncurses.h>

char dungeon[HEIGHT][WIDTH];                 
unsigned char hardness[HEIGHT][WIDTH];     
struct Room rooms[MAX_ROOMS];                
int distance_non_tunnel[HEIGHT][WIDTH];
int distance_tunnel[HEIGHT][WIDTH];

Monster **monsters = NULL;
int num_monsters = 0;

Monster *monsterAt[HEIGHT][WIDTH];

int player_x;
int player_y;
int num_rooms = 0;
int randRoomNum = 0;
int upStairsCount = 0;
int downStairsCount = 0;

int player_room_index = -1;

struct Stairs upStairs[MAX_ROOMS];
struct Stairs downStairs[MAX_ROOMS];

// Define these globally (remove static)
int fog_enabled = 0;
char visible[HEIGHT][WIDTH];
char terrain[HEIGHT][WIDTH];

void printDungeon() {
    for (int y = 0; y < HEIGHT; y++) {
        for (int x = 0; x < WIDTH; x++) {
            if (monsterAt[y][x]) {
                int personality = monsterAt[y][x]->intelligent +
                                  (monsterAt[y][x]->telepathic << 1) +
                                  (monsterAt[y][x]->tunneling << 2) +
                                  (monsterAt[y][x]->erratic << 3);
                char symbol;
                if (personality < 10) {
                    symbol = '0' + personality;
                } else {
                    symbol = 'A' + (personality - 10);
                }
                printf("%c", symbol);
            } else if (x == player_x && y == player_y) {
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
            if (x == 0 || x == WIDTH - 1 || y == 0 || y == HEIGHT - 1) {
                dungeon[y][x] = '+';
            } else {
                dungeon[y][x] = ' ';
            }
        }
    }
}

int overlapCheck(struct Room r1, struct Room r2) {
    if (r1.x + r1.width < r2.x || r2.x + r2.width < r1.x || r1.y + r1.height < r2.y || r2.y + r2.height < r1.y) {
        return 0;
    } else {
        return 1;
    }
}

void createRooms() {
    num_rooms = 0;
    int attempts = 0;
    randRoomNum = MIN_ROOMS + rand() % (MAX_ROOMS - MIN_ROOMS + 1);

    while (num_rooms < randRoomNum && attempts < 1000) {
        attempts++;
        int overlap = 0;
        struct Room newRoom;

        newRoom.width = MIN_ROOM_WIDTH + rand() % (MAX_ROOM_WIDTH - MIN_ROOM_WIDTH + 1);
        newRoom.height = MIN_ROOM_HEIGHT + rand() % (MAX_ROOM_HEIGHT - MIN_ROOM_HEIGHT + 1);
        newRoom.x = 1 + rand() % (WIDTH - newRoom.width - 2);
        newRoom.y = 1 + rand() % (HEIGHT - newRoom.height - 2);

        for (int i = 0; i < num_rooms; i++) {
            if (overlapCheck(newRoom, rooms[i])) {
                overlap = 1;
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
            if (dungeon[y1][x1] == ' ') {
                dungeon[y1][x1] = '#';
            }

            if (rand() % 2) {
                if (x1 < x2) {
                    x1++;
                } else if (x1 > x2) {
                    x1--;
                }
            } else {
                if (y1 < y2) {
                    y1++;
                } else if (y1 > y2) {
                    y1--;
                }
            }
        }
    }
}

void placeStairs() {
    upStairsCount = 1;
    downStairsCount = 1;

    int upIndex = rand() % num_rooms;
    int downIndex = rand() % num_rooms;

    while (downIndex == upIndex) {
        downIndex = rand() % num_rooms;
    }

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
            } else if (dungeon[y][x] == '.' || dungeon[y][x] == '#' || dungeon[y][x] == '@' 
            || dungeon[y][x] == '<' || dungeon[y][x] == '>') {
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

    player_x = playerRoom.x + rand() % playerRoom.width;
    player_y = playerRoom.y + rand() % playerRoom.height;
    dungeon[player_y][player_x] = '@';
}

void movePlayer(void) {
    int curr_x = player_x;
    int curr_y = player_y;
    int next_x = curr_x;
    int next_y = curr_y;

    int dx[] = {-1, 0, 1, -1, 1, -1, 0, 1};
    int dy[] = {-1, -1, -1, 0, 0, 1, 1, 1};

    int dir = rand() % 8;
    int nx = curr_x + dx[dir];
    int ny = curr_y + dy[dir];

    if (nx >= 0 && nx < WIDTH && ny >= 0 && ny < HEIGHT &&
        hardness[ny][nx] == 0 && !monsterAt[ny][nx]) {
        next_x = nx;
        next_y = ny;
    }

    if (next_x != curr_x || next_y != curr_y) {
        char original_terrain;

        if (curr_y == upStairs[0].y && curr_x == upStairs[0].x) {
            original_terrain = '<';
        } else if (curr_y == downStairs[0].y && curr_x == downStairs[0].x) {
            original_terrain = '>';
        } else {
            int in_room = 0;
            for (int i = 0; i < num_rooms; i++) {
                if (curr_x >= rooms[i].x && curr_x < rooms[i].x + rooms[i].width &&
                    curr_y >= rooms[i].y && curr_y < rooms[i].y + rooms[i].height) {
                    in_room = 1;
                    break;
                }
            }
            if (in_room) {
                original_terrain = '.';
            } else {
                original_terrain = '#';
            }
        }

        dungeon[curr_y][curr_x] = original_terrain;

        player_x = next_x;
        player_y = next_y;
        dungeon[player_y][player_x] = '@';
    }
}

Monster *generateMonsterByType(char c, int x, int y) {
    Monster *monster = malloc(sizeof(Monster));
    if (!monster) {
        fprintf(stderr, "Error: Failed to allocate memory for monster\n");
        return NULL;
    }
    monster->x = x;
    monster->y = y;
    monster->speed = rand() % 16 + 5;

    int num;
    c = tolower(c);
    if (c >= '0' && c <= '9') {
        num = c - '0';
    } else if (c >= 'a' && c <= 'f') {
        num = c - 'a' + 10;
    } else {
        printf("Error: Invalid hex character '%c'\n", c);
        free(monster);
        return NULL;
    }

    monster->intelligent = num & 1;
    monster->telepathic = (num >> 1) & 1;
    monster->tunneling = (num >> 2) & 1;
    monster->erratic = (num >> 3) & 1;
    monster->alive = 1;
    monster->last_seen_x = -1;
    monster->last_seen_y = -1;

    return monster;
}

Monster *generateMonster(int x, int y) {
    Monster *monster = malloc(sizeof(Monster));
    if (!monster) {
        fprintf(stderr, "Error: Failed to allocate memory for monster\n");
        return NULL;
    }

    monster->intelligent = rand() % 2;
    monster->tunneling = rand() % 2;
    monster->telepathic = rand() % 2;
    monster->erratic = rand() % 2;
    monster->speed = rand() % 16 + 5;
    monster->x = x;
    monster->y = y;
    monster->alive = 1;
    monster->last_seen_x = -1;
    monster->last_seen_y = -1;

    return monster;
}

void relocateMonster(Monster *monster) {
    if (!monster->alive) return;

    int dist[HEIGHT][WIDTH];
    if (monster->tunneling) {
        dijkstraTunneling(dist);
    } else {
        dijkstraNonTunneling(dist);
    }

    int curr_x = monster->x;
    int curr_y = monster->y;
    int min_dist = dist[curr_y][curr_x];
    int next_x = curr_x;
    int next_y = curr_y;

    int dx[] = {-1, 0, 1, -1, 1, -1, 0, 1};
    int dy[] = {-1, -1, -1, 0, 0, 1, 1, 1};

    for (int i = 0; i < 8; i++) {
        int nx = curr_x + dx[i];
        int ny = curr_y + dy[i];
        
        if (nx >= 0 && nx < WIDTH && ny >= 0 && ny < HEIGHT) {
            if (dist[ny][nx] < min_dist && 
                (monster->tunneling || hardness[ny][nx] == 0) && 
                !monsterAt[ny][nx]) {
                min_dist = dist[ny][nx];
                next_x = nx;
                next_y = ny;
            }
        }
    }

    if (next_x != curr_x || next_y != curr_y) {
        monsterAt[curr_y][curr_x] = NULL;
        monster->x = next_x;
        monster->y = next_y;
        monsterAt[next_y][next_x] = monster;
    }
}

int spawnMonsterByType(char monType) {
    Monster **temp = realloc(monsters, (num_monsters + 1) * sizeof(Monster *));
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
        int in_player_room = (x >= playerRoom.x && x < playerRoom.x + playerRoom.width &&
                              y >= playerRoom.y && y < playerRoom.y + playerRoom.height);
        if (dungeon[y][x] != '.' || 
            (x == player_x && y == player_y) || 
            monsterAt[y][x] || 
            in_player_room) {
            continue;
        }
        
        monsters[num_monsters] = generateMonsterByType(monType, x, y);
        if (!monsters[num_monsters]) {
            return 1;
        }
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
                monsterAt[monsters[i]->y][monsters[i]->x] = NULL;
            }
            if (monsters[i]) free(monsters[i]);
        }
        free(monsters);
        monsters = NULL;
    }

    num_monsters = 0;
    monsters = malloc(numMonsters * sizeof(Monster *));
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
            int in_player_room = (x >= playerRoom.x && x < playerRoom.x + playerRoom.width &&
                                  y >= playerRoom.y && y < playerRoom.y + playerRoom.height);
            if (dungeon[y][x] == '.' && 
                !(x == player_x && y == player_y) && 
                !monsterAt[y][x] && 
                !in_player_room) {
                Monster **temp = realloc(monsters, (num_monsters + 1) * sizeof(Monster *));
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
            int in_player_room = (x >= playerRoom.x && x < playerRoom.x + playerRoom.width &&
                                  y >= playerRoom.y && y < playerRoom.y + playerRoom.height);
            if (dungeon[y][x] == '.' && 
                !(x == player_x && y == player_y) && 
                !monsterAt[y][x] && 
                !in_player_room) {
                Monster **temp = realloc(monsters, (num_monsters + 1) * sizeof(Monster *));
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
                relocateMonster(monsters[i]);
                if (monsters[i]->alive) monsters_alive++;
            }
        }
        sleep(1);
    }
    
    printf("\nFinal state:\n");
    printDungeon();
}

int gameOver(Monster **culprit) {
    for (int i = 0; i < num_monsters; i++) {
        if (monsters[i] && monsters[i]->alive && 
            monsters[i]->x == player_x && monsters[i]->y == player_y) {
            *culprit = monsters[i];
            return 1;
        }
    }
    *culprit = NULL;
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
    const int radius = 5;
    for (int y = 0; y < HEIGHT; y++) {
        for (int x = 0; x < WIDTH; x++) {
            int dx = x - player_x;
            int dy = y - player_y;
            if (dx * dx + dy * dy <= radius * radius) {
                visible[y][x] = 1;
            }
        }
    }
}

void draw_dungeon(WINDOW *win, const char *message) {
    werase(win);
    mvwprintw(win, 0, 0, "%s", message ? message : "");

    for (int y = 0; y < HEIGHT; y++) {
        for (int x = 0; x < WIDTH; x++) {
            if (!fog_enabled || visible[y][x]) {
                if (monsterAt[y][x]) {
                    int personality = monsterAt[y][x]->intelligent +
                                      (monsterAt[y][x]->telepathic << 1) +
                                      (monsterAt[y][x]->tunneling << 2) +
                                      (monsterAt[y][x]->erratic << 3);
                    char symbol = personality < 10 ? '0' + personality : 'A' + (personality - 10);
                    mvwprintw(win, y + 1, x, "%c", symbol);
                } else if (x == player_x && y == player_y) {
                    mvwprintw(win, y + 1, x, "@");
                } else {
                    mvwprintw(win, y + 1, x, "%c", dungeon[y][x]);
                }
            } else {
                mvwprintw(win, y + 1, x, " ");
            }
        }
    }
    mvwprintw(win, 22, 0, "Status Line 1");
    mvwprintw(win, 23, 0, "Fog of War: %s", fog_enabled ? "ON" : "OFF");
    wrefresh(win);
}

void draw_monster_list(WINDOW *win) {
    werase(win);
    int start = 0;
    int max_lines = 22;
    int ch;

    while (1) {
        werase(win);
        mvwprintw(win, 0, 0, "Monster List (Press ESC to exit):");
        for (int i = start; i < num_monsters && i - start < max_lines - 1; i++) {
            if (monsters[i]->alive) {
                int dx = monsters[i]->x - player_x;
                int dy = monsters[i]->y - player_y;
                int personality = monsters[i]->intelligent +
                                  (monsters[i]->telepathic << 1) +
                                  (monsters[i]->tunneling << 2) +
                                  (monsters[i]->erratic << 3);
                char symbol = personality < 10 ? '0' + personality : 'A' + (personality - 10);
                const char *ns = dy < 0 ? "north" : "south";
                const char *ew = dx < 0 ? "west" : "east";
                mvwprintw(win, i - start + 1, 0, "%c, %d %s and %d %s", 
                          symbol, abs(dy), ns, abs(dx), ew);
            }
        }
        wrefresh(win);

        ch = getch();
        if (ch == 27) {
            break;
        } else if (ch == KEY_UP && start > 0) {
            start--;
        } else if (ch == KEY_DOWN && start + max_lines - 1 < num_monsters) {
            start++;
        }
    }
}

void regenerate_dungeon(int numMonsters) {
    for (int i = 0; i < num_monsters; i++) {
        if (monsters[i]) {
            if (monsterAt[monsters[i]->y][monsters[i]->x] == monsters[i]) {
                monsterAt[monsters[i]->y][monsters[i]->x] = NULL;
            }
            free(monsters[i]);
        }
    }
    free(monsters);
    monsters = NULL;
    num_monsters = 0;

    if (player_x >= 0 && player_y >= 0 && player_x < WIDTH && player_y < HEIGHT) {
        dungeon[player_y][player_x] = terrain[player_y][player_x] == 0 ? '.' : terrain[player_y][player_x];
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
    update_visibility();
}

int move_player(int dx, int dy, const char **message) {
    int new_x = player_x + dx;
    int new_y = player_y + dy;
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
    dungeon[player_y][player_x] = terrain[player_y][player_x];
    player_x = new_x;
    player_y = new_y;
    
    if (terrain[player_y][player_x] == 0) {
        terrain[player_y][player_x] = dungeon[player_y][player_x];
    }
    dungeon[player_y][player_x] = '@';
    *message = "";
    update_visibility();
    return 1;
}

int use_stairs(char direction, int numMonsters, const char **message) {
    if (direction == '>' && terrain[player_y][player_x] == '>') {
        regenerate_dungeon(numMonsters);
        *message = "Descended to a new level!";
        return 1;
    } else if (direction == '<' && terrain[player_y][player_x] == '<') {
        regenerate_dungeon(numMonsters);
        *message = "Ascended to a new level!";
        return 1;
    }
    *message = "Not standing on the correct staircase!";
    return 0;
}