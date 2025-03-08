#include "dungeon_generation.h"

char dungeon[HEIGHT][WIDTH];                 
unsigned char hardness[HEIGHT][WIDTH];     
struct Room rooms[MAX_ROOMS];                
int distance_non_tunnel[HEIGHT][WIDTH];
int distance_tunnel[HEIGHT][WIDTH];

int player_x;
int player_y;
int num_rooms = 0;
int randRoomNum = 0;
int upStairsCount = 0;
int downStairsCount = 0;

struct Stairs upStairs[MAX_ROOMS];
struct Stairs downStairs[MAX_ROOMS];

char *dungeonFile;

void printDungeon() {
    for (int y = 0; y < HEIGHT; y++) {
        for (int x = 0; x < WIDTH; x++) {
            if (monsterAt[y][x]) {
                int personality = monsterAt[y][x]->intelligent + (monsterAt[y][x]->telepathic << 1) +
                                  (monsterAt[y][x]->tunneling << 2) + (monsterAt[y][x]->erratic << 3);
                printf("%c", personality < 10 ? '0' + personality : 'a' + (personality - 10));
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


void placePlayer() {
    int index = rand() % num_rooms;
    struct Room playerRoom = rooms[index];

    player_x = playerRoom.x + rand() % playerRoom.width;
    player_y = playerRoom.y + rand() % playerRoom.height;
    dungeon[player_y][player_x] = '@';
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



Monster* createNonTunnelingMonster(int x, int y) {
    Monster* m = malloc(sizeof(Monster));
    if (!m) {
        fprintf(stderr, "Error: Failed to allocate memory for non-tunneling monster\n");
        return NULL;
    }
    m->x = x;
    m->y = y;
    m->speed = 5 + rand() % 16;
    m->intelligent = rand() % 2;
    m->telepathic = rand() % 2;
    m->tunneling = 0;
    m->erratic = rand() % 2;
    return m;
}

Monster* createTunnelingMonster(int x, int y) {
    Monster* m = malloc(sizeof(Monster));
    if (!m) {
        fprintf(stderr, "Error: Failed to allocate memory for tunneling monster\n");
        return NULL;
    }
    m->x = x;
    m->y = y;
    m->speed = 5 + rand() % 16;
    m->intelligent = rand() % 2;
    m->telepathic = rand() % 2;
    m->tunneling = 1;
    m->erratic = rand() % 2;
    return m;
}

int spawnMonsters(int numMonsters) {
    if (numMonsters > HEIGHT * WIDTH) numMonsters = HEIGHT * WIDTH;
    num_monsters = 0;
    int half = numMonsters / 2;

    for (int i = 0; i < numMonsters; i++) {
        int placed = 0;
        for (int attempts = 0; attempts < 100 && !placed; attempts++) {
            int roomIdx = rand() % num_rooms;
            struct Room r = rooms[roomIdx];
            int x = r.x + rand() % r.width;
            int y = r.y + rand() % r.height;
            if (dungeon[y][x] == '.' && !(x == player_x && y == player_y) && !monsterAt[y][x]) {
                Monster* m = (i < half) ? createNonTunnelingMonster(x, y) : createTunnelingMonster(x, y);
                if (!m) return 1;
                monsters[num_monsters] = m;
                monsterAt[y][x] = m;
                num_monsters++;
                placed = 1;
            }
        }
        if (!placed) {
            fprintf(stderr, "Error: Failed to spawn monster %d\n", i);
            return 1;
        }
    }
    return 0;
}

int populateDungeon(int numMonsters) {
    return spawnMonsters(numMonsters);
}

int fillDungeon(int numMonsters) {
    emptyDungeon();
    createRooms();
    connectRooms();
    placeStairs();
    placePlayer();
    initializeHardness();
    return spawnMonsters(numMonsters);
}

void cleanup() {
    for (int i = 0; i < num_monsters; i++) {
        if (monsters[i]) free(monsters[i]);
    }
    num_monsters = 0;
    for (int y = 0; y < HEIGHT; y++) {
        for (int x = 0; x < WIDTH; x++) {
            monsterAt[y][x] = NULL;
        }
    }
}