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



void printNonTunnelingMap() {
    int dist[HEIGHT][WIDTH];
    dijkstraNonTunneling(dist);
    for (int y = 0; y < HEIGHT; y++) {
        for (int x = 0; x < WIDTH; x++) {
            if (x == player_x && y == player_y) {
                printf("@");
            } else if (dist[y][x] == INFINITY) {
                printf("%c", dungeon[y][x]);
            } else {
                printf("%d", dist[y][x] % 10);
            }
        }
        printf("\n");
    }
}

void printTunnelingMap() {
    int dist[HEIGHT][WIDTH];
    dijkstraTunneling(dist);
    for (int y = 0; y < HEIGHT; y++) {
        for (int x = 0; x < WIDTH; x++) {
            if (x == player_x && y == player_y) {
                printf("@");
            } else if (dist[y][x] == INFINITY) {
                printf("%c", dungeon[y][x]);
            } else {
                printf("%d", dist[y][x] % 10);
            }
        }
        printf("\n");
    }
}

void spawnMonsters(int count) {
    num_monsters = count;
    monsters = malloc(num_monsters * sizeof(Monster));
    if (!monsters) {
        fprintf(stderr, "Error: Failed to allocate memory for monsters\n");
        exit(1);
    }

    for (int i = 0; i < num_monsters; i++) {
        int placed = 0;
        while (!placed) {
            int room_idx = rand() % num_rooms;
            int x = rooms[room_idx].x + rand() % rooms[room_idx].width;
            int y = rooms[room_idx].y + rand() % rooms[room_idx].height;
            if (hardness[y][x] == 0 && !(x == player_x && y == player_y)) {
                int occupied = 0;
                for (int j = 0; j < i; j++) {
                    if (monsters[j].pos.x == x && monsters[j].pos.y == y) {
                        occupied = 1;
                        break;
                    }
                }
                if (!occupied) {
                    monsters[i] = (Monster){
                        .pos = {x, y},
                        .speed = 5 + rand() % 16, // 5 to 20
                        .intelligent = rand() % 2,
                        .telepathic = rand() % 2,
                        .tunneling = rand() % 2,
                        .erratic = rand() % 2,
                        .last_known_pc = {-1, -1},
                        .alive = 1
                    };
                    placed = 1;
                }
            }
        }
    }
}

static int hasLineOfSight(int x1, int y1, int x2, int y2) {
    int dx = abs(x2 - x1), dy = abs(y2 - y1);
    int sx = (x1 < x2) ? 1 : -1, sy = (y1 < y2) ? 1 : -1;
    int err = dx - dy;
    while (x1 != x2 || y1 != y2) {
        if (hardness[y1][x1] > 0) return 0; // Blocked by rock or wall
        int e2 = 2 * err;
        if (e2 > -dy) { err -= dy; x1 += sx; }
        if (e2 < dx) { err += dx; y1 += sy; }
    }
    return 1;
}

void runGame() {
    int dist_non_tunnel[HEIGHT][WIDTH], dist_tunnel[HEIGHT][WIDTH];
    dijkstraNonTunneling(dist_non_tunnel);
    dijkstraTunneling(dist_tunnel);

    MinHeap* event_queue = createMinHeap(HEIGHT * WIDTH);
    if (!event_queue) {
        fprintf(stderr, "Error: Failed to create event queue\n");
        free(monsters);
        return;
    }

    // Insert PC event (speed 10, moves every 100 turns)
    insertHeap(event_queue, (HeapNode){player_x, player_y, 0});
    // Insert monster events
    for (int i = 0; i < num_monsters; i++) {
        insertHeap(event_queue, (HeapNode){monsters[i].pos.x, monsters[i].pos.y, 0});
    }

    int game_time = 0;
    int monsters_alive = num_monsters;

    while (event_queue->size > 0) {
        HeapNode event = extractMin(event_queue);
        game_time = event.distance;

        // PC movement
        if (event.x == player_x && event.y == player_y) {
            int dx[] = {-1, 0, 1, -1, 1, -1, 0, 1}, dy[] = {-1, -1, -1, 0, 0, 1, 1, 1};
            int moved = 0;
            for (int i = rand() % 8; i < 8 && !moved; i++) {
                int nx = player_x + dx[i], ny = player_y + dy[i];
                if (nx >= 0 && nx < WIDTH && ny >= 0 && ny < HEIGHT && hardness[ny][nx] != 255) {
                    int monster_idx = -1;
                    for (int j = 0; j < num_monsters; j++) {
                        if (monsters[j].alive && monsters[j].pos.x == nx && monsters[j].pos.y == ny) {
                            monster_idx = j;
                            break;
                        }
                    }
                    if (monster_idx != -1) {
                        monsters[monster_idx].alive = 0;
                        monsters_alive--;
                        printf("Player killed monster, %d monsters left\n", monsters_alive);
                    }
                    dungeon[player_y][player_x] = (hardness[player_y][player_x] == 0) ? '.' : '#';
                    player_x = nx;
                    player_y = ny;
                    dungeon[player_y][player_x] = '@';
                    moved = 1;
                    dijkstraNonTunneling(dist_non_tunnel);
                    dijkstraTunneling(dist_tunnel);
                }
            }
            insertHeap(event_queue, (HeapNode){player_x, player_y, game_time + 100}); // Speed 10
            printDungeon();
            usleep(250000); // 250ms pause
            if (monsters_alive == 0) {
                printf("Player wins!\n");
                break;
            }
        } else {
            // Monster movement
            int monster_idx = -1;
            for (int i = 0; i < num_monsters; i++) {
                if (monsters[i].alive && monsters[i].pos.x == event.x && monsters[i].pos.y == event.y) {
                    monster_idx = i;
                    break;
                }
            }
            if (monster_idx == -1) continue;

            Monster* m = &monsters[monster_idx];
            int target_x = player_x, target_y = player_y;

            // Determine target based on telepathy and line of sight
            if (!m->telepathic) {
                if (!hasLineOfSight(m->pos.x, m->pos.y, player_x, player_y)) {
                    if (m->intelligent && m->last_known_pc.x != -1) {
                        target_x = m->last_known_pc.x;
                        target_y = m->last_known_pc.y;
                    } else {
                        insertHeap(event_queue, (HeapNode){m->pos.x, m->pos.y, game_time + 1000 / m->speed});
                        continue; // Stay still
                    }
                } else {
                    m->last_known_pc = (Pos){player_x, player_y};
                }
            }

            int nx = m->pos.x, ny = m->pos.y;
            int dx[] = {-1, 0, 1, -1, 1, -1, 0, 1}, dy[] = {-1, -1, -1, 0, 0, 1, 1, 1};

            if (m->erratic && rand() % 2) {
                // Erratic: random move
                int i = rand() % 8;
                nx = m->pos.x + dx[i];
                ny = m->pos.y + dy[i];
                if (nx < 0 || nx >= WIDTH || ny < 0 || ny >= HEIGHT || 
                    (!m->tunneling && hardness[ny][nx] != 0) || hardness[ny][nx] == 255) {
                    nx = m->pos.x;
                    ny = m->pos.y; // Stay still if invalid
                }
            } else if (m->intelligent) {
                // Intelligent: use tunneling map
                int min_dist = dist_tunnel[m->pos.y][m->pos.x];
                for (int i = 0; i < 8; i++) {
                    int tx = m->pos.x + dx[i], ty = m->pos.y + dy[i];
                    if (tx >= 0 && tx < WIDTH && ty >= 0 && ty < HEIGHT && 
                        dist_tunnel[ty][tx] < min_dist && hardness[ty][tx] != 255) {
                        min_dist = dist_tunnel[ty][tx];
                        nx = tx;
                        ny = ty;
                    }
                }
            } else {
                // Dumb: use non-tunneling map
                int min_dist = dist_non_tunnel[m->pos.y][m->pos.x];
                for (int i = 0; i < 8; i++) {
                    int tx = m->pos.x + dx[i], ty = m->pos.y + dy[i];
                    if (tx >= 0 && tx < WIDTH && ty >= 0 && ty < HEIGHT && 
                        dist_non_tunnel[ty][tx] < min_dist && hardness[ty][tx] == 0) {
                        min_dist = dist_non_tunnel[ty][tx];
                        nx = tx;
                        ny = ty;
                    }
                }
            }

            // Handle tunneling
            if (m->tunneling && hardness[ny][nx] > 0 && hardness[ny][nx] != 255) {
                hardness[ny][nx] = (hardness[ny][nx] <= 85) ? 0 : hardness[ny][nx] - 85;
                if (hardness[ny][nx] == 0) {
                    dungeon[ny][nx] = '#';
                    dijkstraTunneling(dist_tunnel);
                    dijkstraNonTunneling(dist_non_tunnel);
                    m->pos.x = nx;
                    m->pos.y = ny;
                }
            } else if (hardness[ny][nx] == 0) {
                m->pos.x = nx;
                m->pos.y = ny;
            }

            // Check for collision with player
            if (m->pos.x == player_x && m->pos.y == player_y) {
                printDungeon();
                printf("Player killed by monster\n");
                break;
            }

            insertHeap(event_queue, (HeapNode){m->pos.x, m->pos.y, game_time + 1000 / m->speed});
        }
    }

    free(event_queue->nodes);
    free(event_queue);
    free(monsters);
}