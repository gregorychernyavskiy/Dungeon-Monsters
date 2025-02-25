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

// Dijkstra's algorithm for non-tunneling monsters

void dijkstraNonTunneling(int dist[HEIGHT][WIDTH]) {
    // Initialize distances to infinity
    for (int y = 0; y < HEIGHT; y++) {
        for (int x = 0; x < WIDTH; x++) {
            dist[y][x] = INFINITY;
        }
    }
    dist[player_y][player_x] = 0;

    // Simple array-based queue (since weight is uniform, this is effectively BFS)
    int queue[HEIGHT * WIDTH][3]; // [x, y, dist]
    int q_size = 0;
    int visited[HEIGHT][WIDTH] = {0};

    // Add player position
    queue[q_size][0] = player_x;
    queue[q_size][1] = player_y;
    queue[q_size][2] = 0;
    q_size++;

    // 8-way connectivity
    int dx[] = {-1, 0, 1, -1, 1, -1, 0, 1};
    int dy[] = {-1, -1, -1, 0, 0, 1, 1, 1};

    while (q_size > 0) {
        // Extract the first element (FIFO for BFS-like behavior)
        int x = queue[0][0];
        int y = queue[0][1];
        int curr_dist = queue[0][2];

        // Remove from queue
        for (int i = 0; i < q_size - 1; i++) {
            queue[i][0] = queue[i + 1][0];
            queue[i][1] = queue[i + 1][1];
            queue[i][2] = queue[i + 1][2];
        }
        q_size--;

        if (visited[y][x]) continue;
        visited[y][x] = 1;

        // Process neighbors
        for (int i = 0; i < 8; i++) {
            int nx = x + dx[i];
            int ny = y + dy[i];

            if (nx < 0 || nx >= WIDTH || ny < 0 || ny >= HEIGHT) continue;
            if (visited[ny][nx]) continue;
            if (hardness[ny][nx] != 0) continue; // Only move through hardness 0 (floor)

            int new_dist = curr_dist + 1; // Uniform weight of 1
            if (new_dist < dist[ny][nx]) {
                dist[ny][nx] = new_dist;
                // Add to queue
                queue[q_size][0] = nx;
                queue[q_size][1] = ny;
                queue[q_size][2] = new_dist;
                q_size++;
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



// Dijkstra's algorithm for tunneling monsters
void dijkstraTunneling(int dist[HEIGHT][WIDTH]) {
    // Initialize distances to infinity
    for (int y = 0; y < HEIGHT; y++) {
        for (int x = 0; x < WIDTH; x++) {
            dist[y][x] = INFINITY;
        }
    }
    dist[player_y][player_x] = 0;

    // Simple array-based priority queue (for simplicity; replace with your own if needed)
    int visited[HEIGHT][WIDTH] = {0};
    int queue[HEIGHT * WIDTH][3]; // [x, y, dist]
    int q_size = 0;

    // Add player position
    queue[q_size][0] = player_x;
    queue[q_size][1] = player_y;
    queue[q_size][2] = 0;
    q_size++;

    // 8-way connectivity
    int dx[] = {-1, 0, 1, -1, 1, -1, 0, 1};
    int dy[] = {-1, -1, -1, 0, 0, 1, 1, 1};

    while (q_size > 0) {
        // Find node with minimum distance (naive approach for simplicity)
        int min_idx = 0;
        for (int i = 1; i < q_size; i++) {
            if (queue[i][2] < queue[min_idx][2]) {
                min_idx = i;
            }
        }

        int x = queue[min_idx][0];
        int y = queue[min_idx][1];
        int curr_dist = queue[min_idx][2];

        // Remove min node from queue
        queue[min_idx][0] = queue[q_size - 1][0];
        queue[min_idx][1] = queue[q_size - 1][1];
        queue[min_idx][2] = queue[q_size - 1][2];
        q_size--;

        if (visited[y][x]) continue;
        visited[y][x] = 1;

        // Process neighbors
        for (int i = 0; i < 8; i++) {
            int nx = x + dx[i];
            int ny = y + dy[i];

            if (nx < 0 || nx >= WIDTH || ny < 0 || ny >= HEIGHT) continue;
            if (visited[ny][nx]) continue;

            // Calculate weight
            int weight;
            if (hardness[ny][nx] == 0) {
                weight = 1;
            } else if (hardness[ny][nx] == 255) {
                continue; // Infinite weight
            } else {
                weight = 1 + hardness[ny][nx] / 85; // Integer division as per spec
            }

            int new_dist = curr_dist + weight;
            if (new_dist < dist[ny][nx]) {
                dist[ny][nx] = new_dist;
                // Add to queue
                queue[q_size][0] = nx;
                queue[q_size][1] = ny;
                queue[q_size][2] = new_dist;
                q_size++;
            }
        }
    }
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