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




MinHeap* createMinHeap(int capacity) {
    MinHeap* heap = malloc(sizeof(MinHeap));
    heap->nodes = malloc(sizeof(HeapNode) * capacity);
    heap->size = 0;
    heap->capacity = capacity;
    return heap;
}

void heapify(MinHeap* heap, int idx) {
    int smallest = idx;
    int left = 2 * idx + 1;
    int right = 2 * idx + 2;

    if (left < heap->size && heap->nodes[left].distance < heap->nodes[smallest].distance)
        smallest = left;
    if (right < heap->size && heap->nodes[right].distance < heap->nodes[smallest].distance)
        smallest = right;

    if (smallest != idx) {
        HeapNode temp = heap->nodes[idx];
        heap->nodes[idx] = heap->nodes[smallest];
        heap->nodes[smallest] = temp;
        heapify(heap, smallest);
    }
}

void insertHeap(MinHeap* heap, HeapNode node) {
    if (heap->size == heap->capacity) return;

    int i = heap->size++;
    heap->nodes[i] = node;

    while (i != 0 && heap->nodes[(i-1)/2].distance > heap->nodes[i].distance) {
        HeapNode temp = heap->nodes[i];
        heap->nodes[i] = heap->nodes[(i-1)/2];
        heap->nodes[(i-1)/2] = temp;
        i = (i-1)/2;
    }
}

HeapNode extractMin(MinHeap* heap) {
    if (heap->size <= 0) {
        HeapNode empty = {-1, -1, -1};
        return empty;
    }
    
    HeapNode root = heap->nodes[0];
    heap->nodes[0] = heap->nodes[--heap->size];
    heapify(heap, 0);
    return root;
}




void dijkstraNonTunneling(int dist[HEIGHT][WIDTH]) {
    
    for (int y = 0; y < HEIGHT; y++) {
        for (int x = 0; x < WIDTH; x++) {
            dist[y][x] = INFINITY;
        }
    }
    dist[player_y][player_x] = 0;

    
    int queue[HEIGHT * WIDTH][3];
    int q_size = 0;
    int visited[HEIGHT][WIDTH] = {0};

    queue[q_size][0] = player_x;
    queue[q_size][1] = player_y;
    queue[q_size][2] = 0;
    q_size++;

    int dx[] = {-1, 0, 1, -1, 1, -1, 0, 1};
    int dy[] = {-1, -1, -1, 0, 0, 1, 1, 1};

    while (q_size > 0) {
        
        int x = queue[0][0];
        int y = queue[0][1];
        int curr_dist = queue[0][2];

        for (int i = 0; i < q_size - 1; i++) {
            queue[i][0] = queue[i + 1][0];
            queue[i][1] = queue[i + 1][1];
            queue[i][2] = queue[i + 1][2];
        }
        q_size--;

        if (visited[y][x]) continue;
        visited[y][x] = 1;

        for (int i = 0; i < 8; i++) {
            int nx = x + dx[i];
            int ny = y + dy[i];

            if (nx < 0 || nx >= WIDTH || ny < 0 || ny >= HEIGHT) continue;
            if (visited[ny][nx]) continue;
            if (hardness[ny][nx] != 0) continue;

            int new_dist = curr_dist + 1;
            if (new_dist < dist[ny][nx]) {
                dist[ny][nx] = new_dist;
                queue[q_size][0] = nx;
                queue[q_size][1] = ny;
                queue[q_size][2] = new_dist;
                q_size++;
            }
        }
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











void dijkstraTunneling(int dist[HEIGHT][WIDTH]) {
    for (int y = 0; y < HEIGHT; y++) {
        for (int x = 0; x < WIDTH; x++) {
            dist[y][x] = INFINITY;
        }
    }
    dist[player_y][player_x] = 0;

    int visited[HEIGHT][WIDTH] = {0};
    int queue[HEIGHT * WIDTH][3];
    int q_size = 0;

    queue[q_size][0] = player_x;
    queue[q_size][1] = player_y;
    queue[q_size][2] = 0;
    q_size++;
    int dx[] = {-1, 0, 1, -1, 1, -1, 0, 1};
    int dy[] = {-1, -1, -1, 0, 0, 1, 1, 1};

    while (q_size > 0) {
        int min_idx = 0;
        for (int i = 1; i < q_size; i++) {
            if (queue[i][2] < queue[min_idx][2]) {
                min_idx = i;
            }
        }

        int x = queue[min_idx][0];
        int y = queue[min_idx][1];
        int curr_dist = queue[min_idx][2];

        queue[min_idx][0] = queue[q_size - 1][0];
        queue[min_idx][1] = queue[q_size - 1][1];
        queue[min_idx][2] = queue[q_size - 1][2];
        q_size--;

        if (visited[y][x]) continue;
        visited[y][x] = 1;

        for (int i = 0; i < 8; i++) {
            int nx = x + dx[i];
            int ny = y + dy[i];

            if (nx < 0 || nx >= WIDTH || ny < 0 || ny >= HEIGHT) continue;
            if (visited[ny][nx]) continue;

            int weight;
            if (hardness[ny][nx] == 0) {
                weight = 1;
            } else if (hardness[ny][nx] == 255) {
                continue;
            } else {
                weight = 1 + hardness[ny][nx] / 85;
            }

            int new_dist = curr_dist + weight;
            if (new_dist < dist[ny][nx]) {
                dist[ny][nx] = new_dist;
                
                queue[q_size][0] = nx;
                queue[q_size][1] = ny;
                queue[q_size][2] = new_dist;
                q_size++;
            }
        }
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