#include "dungeon_generation.h"
#include <stdlib.h>
#include <string.h>

static void swap(HeapNode* a, HeapNode* b) {
    HeapNode temp = *a;
    *a = *b;
    *b = temp;
}

static void heapifyUp(MinHeap* heap, int index) {
    while (index > 0) {
        int parent = (index - 1) / 2;
        if (heap->nodes[parent].cost > heap->nodes[index].cost) {
            swap(&heap->nodes[parent], &heap->nodes[index]);
            index = parent;
        } else {
            break;
        }
    }
}

static void heapifyDown(MinHeap* heap, int index) {
    int smallest = index;
    int left = 2 * index + 1;
    int right = 2 * index + 2;

    if (left < heap->size && heap->nodes[left].cost < heap->nodes[smallest].cost) {
        smallest = left;
    }
    if (right < heap->size && heap->nodes[right].cost < heap->nodes[smallest].cost) {
        smallest = right;
    }

    if (smallest != index) {
        swap(&heap->nodes[index], &heap->nodes[smallest]);
        heapifyDown(heap, smallest);
    }
}

MinHeap* createMinHeap(int capacity) {
    MinHeap* heap = malloc(sizeof(MinHeap));
    heap->nodes = malloc(sizeof(HeapNode) * capacity);
    heap->size = 0;
    heap->capacity = capacity;
    return heap;
}

void destroyMinHeap(MinHeap* heap) {
    free(heap->nodes);
    free(heap);
}

void heapPush(MinHeap* heap, int x, int y, uint32_t cost) {
    if (heap->size == heap->capacity) {
        return; // Heap is full
    }
    
    HeapNode node = {x, y, cost};
    heap->nodes[heap->size] = node;
    heapifyUp(heap, heap->size);
    heap->size++;
}

HeapNode heapPop(MinHeap* heap) {
    if (heap->size == 0) {
        HeapNode empty = {-1, -1, INFINITY};
        return empty;
    }

    HeapNode root = heap->nodes[0];
    heap->size--;
    heap->nodes[0] = heap->nodes[heap->size];
    heapifyDown(heap, 0);
    return root;
}

void dijkstraNonTunneling(int dist[HEIGHT][WIDTH]) {
    for (int y = 0; y < HEIGHT; y++) {
        for (int x = 0; x < WIDTH; x++) {
            dist[y][x] = INFINITY;
        }
    }
    dist[player_y][player_x] = 0;

    int queue[HEIGHT * WIDTH][2]; // [x, y]
    int front = 0, rear = 0;
    int visited[HEIGHT][WIDTH] = {0};

    queue[rear][0] = player_x;
    queue[rear][1] = player_y;
    rear++;

    int dx[] = {-1, 0, 1, -1, 1, -1, 0, 1};
    int dy[] = {-1, -1, -1, 0, 0, 1, 1, 1};

    while (front < rear) {
        int x = queue[front][0];
        int y = queue[front][1];
        front++;

        if (visited[y][x]) continue;
        visited[y][x] = 1;

        for (int i = 0; i < 8; i++) {
            int nx = x + dx[i];
            int ny = y + dy[i];

            if (nx < 0 || nx >= WIDTH || ny < 0 || ny >= HEIGHT) continue;
            if (visited[ny][nx] || hardness[ny][nx] != 0) continue;

            if (dist[ny][nx] == INFINITY) {
                dist[ny][nx] = dist[y][x] + 1;
                queue[rear][0] = nx;
                queue[rear][1] = ny;
                rear++;
            }
        }
    }
}

// Print non-tunneling distance map
void printNonTunnelingMap() {
    int dist[HEIGHT][WIDTH];
    dijkstraNonTunneling(dist);

    for (int y = 0; y < HEIGHT; y++) {
        for (int x = 0; x < WIDTH; x++) {
            if (x == player_x && y == player_y) {
                printf("@");
            } else if (dist[y][x] == INFINITY) {
                printf("%c", dungeon[y][x]); // Unreachable areas show dungeon terrain
            } else {
                printf("%d", dist[y][x] % 10); // Last digit of distance
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

    MinHeap* heap = createMinHeap(HEIGHT * WIDTH);
    int visited[HEIGHT][WIDTH] = {0};

    HeapNode start = {player_x, player_y, 0};
    insertHeap(heap, start);

    int dx[] = {-1, 0, 1, -1, 1, -1, 0, 1};
    int dy[] = {-1, -1, -1, 0, 0, 1, 1, 1};

    while (heap->size > 0) {
        HeapNode curr = extractMin(heap);
        int x = curr.x;
        int y = curr.y;
        int curr_dist = curr.distance;

        if (visited[y][x]) continue;
        visited[y][x] = 1;

        for (int i = 0; i < 8; i++) {
            int nx = x + dx[i];
            int ny = y + dy[i];

            if (nx < 0 || nx >= WIDTH || ny < 0 || ny >= HEIGHT) continue;
            if (visited[ny][nx]) continue;
            if (hardness[ny][nx] == 255) continue; // Infinite weight

            int weight = (hardness[ny][nx] == 0) ? 1 : 1 + hardness[ny][nx] / 85;
            int new_dist = curr_dist + weight;

            if (new_dist < dist[ny][nx]) {
                dist[ny][nx] = new_dist;
                HeapNode next = {nx, ny, new_dist};
                insertHeap(heap, next);
            }
        }
    }

    free(heap->nodes);
    free(heap);
}

// Print tunneling distance map
void printTunnelingMap() {
    int dist[HEIGHT][WIDTH];
    dijkstraTunneling(dist);

    for (int y = 0; y < HEIGHT; y++) {
        for (int x = 0; x < WIDTH; x++) {
            if (x == player_x && y == player_y) {
                printf("@");
            } else if (dist[y][x] == INFINITY) {
                printf("%c", dungeon[y][x]); // Unreachable areas show dungeon terrain
            } else {
                printf("%d", dist[y][x] % 10); // Last digit of distance
            }
        }
        printf("\n");
    }
}