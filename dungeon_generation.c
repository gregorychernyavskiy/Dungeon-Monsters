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


void printDungeon() {
    for (int y = 0; y < HEIGHT; y++) {
        for (int x = 0; x < WIDTH; x++) {
            printf("%c", dungeon[y][x]);
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

void movePlayerRandom() {
    if (!pc.alive) return;

    int dx[] = {-1, 0, 1, -1, 1, -1, 0, 1, 0};
    int dy[] = {-1, -1, -1, 0, 0, 1, 1, 1, 0};
    int dir = rand() % 9;
    int new_x = pc.x + dx[dir];
    int new_y = pc.y + dy[dir];

    if (new_x >= 0 && new_x < WIDTH && new_y >= 0 && new_y < HEIGHT && hardness[new_y][new_x] != 255) {
        // Check for monster collision
        for (int i = 0; i < num_monsters; i++) {
            if (monsters[i].alive && monsters[i].x == new_x && monsters[i].y == new_y) {
                monsters[i].alive = 0; // PC kills monster
                printf("Player killed monster %c at (%d, %d)\n", monsters[i].symbol, new_x, new_y);
                break;
            }
        }
        // Move PC
        dungeon[pc.y][pc.x] = (hardness[pc.y][pc.x] == 0) ? '.' : '#';
        pc.x = new_x;
        pc.y = new_y;
        dungeon[pc.y][pc.x] = '@';
    }
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
