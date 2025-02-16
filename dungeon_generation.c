#include "dungeon_generation.h"

int player_x, player_y;
int num_rooms = 0;

char dungeon[HEIGHT][WIDTH];
unsigned char hardness[HEIGHT][WIDTH];
struct Room rooms[MAX_ROOMS];


void printDungeon() {
    for(int y = 0; y < HEIGHT; y++) {
        for(int x = 0; x < WIDTH; x++) {
            printf("%c", dungeon[y][x]);
        }
        printf("\n");
    }
}


void emptyDungeon() {
    for(int y = 0; y < HEIGHT; y++) {
        for(int x = 0; x < WIDTH; x++) {
            if(x == 0 || x == WIDTH - 1 || y == 0 || y == HEIGHT - 1) {
                dungeon[y][x] = '+';
            } else {
                dungeon[y][x] = ' ';
            }
        }
    }
}


int overlapCheck(struct Room r1, struct Room r2) {
    if(r1.x + r1.width < r2.x || r2.x + r2.width < r1.x || r1.y + r1.height < r2.y || r2.y + r2.height < r1.y) {
        return 0;
    } else {
        return 1;
    }
}


int createRooms() {
    int countRooms = 0;
    int attempts = 0;
    int randRoomNum = MIN_ROOMS + rand() % (MAX_ROOMS - MIN_ROOMS + 1);

    while(countRooms < randRoomNum && attempts < 1000) {
        attempts++;
        int overlap = 0;
        struct Room newRoom;

        newRoom.width = MIN_ROOM_WIDTH + rand() % (MAX_ROOM_WIDTH - MIN_ROOM_WIDTH + 1);
        newRoom.height = MIN_ROOM_HEIGHT + rand() % (MAX_ROOM_HEIGHT - MIN_ROOM_HEIGHT + 1);
        newRoom.x = 1 + rand() % (WIDTH - newRoom.width - 2);
        newRoom.y = 1 + rand() % (HEIGHT - newRoom.height - 2);

        for(int i = 0; i < countRooms; i++) {
            if(overlapCheck(newRoom, rooms[i])) {
                overlap = 1;
                break;
            }
        }

        if(!overlap) {
            rooms[countRooms++] = newRoom;
            for(int y = newRoom.y; y < newRoom.y + newRoom.height; y++) {
                for(int x = newRoom.x; x < newRoom.x + newRoom.width; x++) {
                    dungeon[y][x] = '.';
                }
            }
        }
    }
    return countRooms;
}


void connectRooms(int countRooms) {
    for(int i = 0; i < countRooms - 1; i++) {
        int x1 = rooms[i].x + rooms[i].width / 2;
        int y1 = rooms[i].y + rooms[i].height / 2;
        int x2 = rooms[i + 1].x + rooms[i + 1].width / 2;
        int y2 = rooms[i + 1].y + rooms[i + 1].height / 2;

        while(x1 != x2 || y1 != y2) {
            if(dungeon[y1][x1] == ' ') {
                dungeon[y1][x1] = '#';
            }

            if(rand() % 2) {
                if(x1 < x2) {
                    x1++;
                } else if(x1 > x2) {
                    x1--;
                }
            } else {
                if(y1 < y2) {
                    y1++;
                } else if(y1 > y2) {
                    y1--;
                }
            }
        }
    }
}


void placeStairs(int countRooms) {
    int upIndex = rand() % countRooms;
    int downIndex = rand() % countRooms;

    while (downIndex == upIndex) {
        downIndex = rand() % countRooms;
    }

    struct Room upRoom = rooms[upIndex];
    struct Room downRoom = rooms[downIndex];

    int upX = upRoom.x + rand() % upRoom.width;
    int upY = upRoom.y + rand() % upRoom.height;
    dungeon[upY][upX] = '<';

    int downX = downRoom.x + rand() % downRoom.width;
    int downY = downRoom.y + rand() % downRoom.height;
    dungeon[downY][downX] = '>';
}


void placePlayer(int countRooms) {
    int index = rand() % countRooms;
    struct Room playerRoom = rooms[index];

    player_x = playerRoom.x + rand() % playerRoom.width;
    player_y = playerRoom.y + rand() % playerRoom.height;
    dungeon[player_y][player_x] = '@';
}