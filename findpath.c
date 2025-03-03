#include "dungeon_generation.h"

static int isValid(int x, int y) {
    return (x >= 0 && x < WIDTH && y >= 0 && y < HEIGHT);
}

MinHeap* createMinHeap(int capacity) {
    MinHeap* heap = malloc(sizeof(MinHeap));
    heap->size = 0;
    heap->capacity = capacity;
    heap->nodes = malloc(capacity * sizeof(HeapNode));
    return heap;
}

void heapify(MinHeap* heap, int idx) {
    int smallest = idx;
    int left = 2 * idx + 1;
    int right = 2 * idx + 2;

    if (left < heap->size && 
        heap->nodes[left].distance < heap->nodes[smallest].distance) {
        smallest = left;
    }

    if (right < heap->size && 
        heap->nodes[right].distance < heap->nodes[smallest].distance) {
        smallest = right;
    }

    if (smallest != idx) {
        HeapNode temp = heap->nodes[idx];
        heap->nodes[idx] = heap->nodes[smallest];
        heap->nodes[smallest] = temp;
        heapify(heap, smallest);
    }
}

void insertHeap(MinHeap* heap, HeapNode node) {
    if (heap->size == heap->capacity) {
        return;
    }

    heap->nodes[heap->size] = node;
    int i = heap->size;
    heap->size++;

    while (i != 0 && heap->nodes[(i-1)/2].distance > heap->nodes[i].distance) {
        HeapNode temp = heap->nodes[i];
        heap->nodes[i] = heap->nodes[(i-1)/2];
        heap->nodes[(i-1)/2] = temp;
        i = (i-1)/2;
    }
}

HeapNode extractMin(MinHeap* heap) {
    if (heap->size <= 0) {
        HeapNode empty = {-1, -1, INFINITY};
        return empty;
    }

    if (heap->size == 1) {
        heap->size--;
        return heap->nodes[0];
    }

    HeapNode root = heap->nodes[0];
    heap->nodes[0] = heap->nodes[heap->size-1];
    heap->size--;
    heapify(heap, 0);

    return root;
}

void decreasePriority(MinHeap* heap, int x, int y, int newDistance) {
    int i;
    for (i = 0; i < heap->size; i++) {
        if (heap->nodes[i].x == x && heap->nodes[i].y == y) {
            break;
        }
    }
    if (i < heap->size && newDistance < heap->nodes[i].distance) {
        heap->nodes[i].distance = newDistance;
        while (i != 0 && heap->nodes[(i-1)/2].distance > heap->nodes[i].distance) {
            HeapNode temp = heap->nodes[i];
            heap->nodes[i] = heap->nodes[(i-1)/2];
            heap->nodes[(i-1)/2] = temp;
            i = (i-1)/2;
        }
    }
}

void dijkstraNonTunneling(int dist[HEIGHT][WIDTH]) {
    int dx[8] = {-1, 0, 1, -1, 1, -1, 0, 1};
    int dy[8] = {-1, -1, -1, 0, 0, 1, 1, 1};
    int visited[HEIGHT][WIDTH] = {0};

    for (int y = 0; y < HEIGHT; y++) {
        for (int x = 0; x < WIDTH; x++) {
            dist[y][x] = INFINITY;
        }
    }
    dist[player_y][player_x] = 0;

    MinHeap* heap = createMinHeap(WIDTH * HEIGHT);
    HeapNode start = {player_x, player_y, 0};
    insertHeap(heap, start);

    while (heap->size > 0) {
        HeapNode current = extractMin(heap);
        int x = current.x;
        int y = current.y;

        if (visited[y][x]) continue;
        visited[y][x] = 1;

        for (int i = 0; i < 8; i++) {
            int new_x = x + dx[i];
            int new_y = y + dy[i];

            if (isValid(new_x, new_y) && hardness[new_y][new_x] == 0) {
                int new_dist = dist[y][x] + 1;
                if (new_dist < dist[new_y][new_x]) {
                    dist[new_y][new_x] = new_dist;
                    if (!visited[new_y][new_x]) {
                        decreasePriority(heap, new_x, new_y, new_dist);
                        HeapNode neighbor = {new_x, new_y, new_dist};
                        insertHeap(heap, neighbor);
                    }
                }
            }
        }
    }
    free(heap->nodes);
    free(heap);
}

void dijkstraTunneling(int dist[HEIGHT][WIDTH]) {
    int dx[8] = {-1, 0, 1, -1, 1, -1, 0, 1};
    int dy[8] = {-1, -1, -1, 0, 0, 1, 1, 1};

    // Initialize distances
    for (int y = 0; y < HEIGHT; y++) {
        for (int x = 0; x < WIDTH; x++) {
            dist[y][x] = INFINITY;
        }
    }
    dist[player_y][player_x] = 0;

    MinHeap* heap = createMinHeap(WIDTH * HEIGHT);
    HeapNode start = {player_x, player_y, 0};
    insertHeap(heap, start);

    while (heap->size > 0) {
        HeapNode current = extractMin(heap);
        int x = current.x;
        int y = current.y;

        for (int i = 0; i < 8; i++) {
            int new_x = x + dx[i];
            int new_y = y + dy[i];

            if (isValid(new_x, new_y) && hardness[new_y][new_x] != 255) {
                int weight = (hardness[new_y][new_x] == 0) ? 1 : 
                            1 + (hardness[new_y][new_x] / 85);
                int new_dist = dist[y][x] + weight;
                if (new_dist < dist[new_y][new_x]) {
                    dist[new_y][new_x] = new_dist;
                    HeapNode neighbor = {new_x, new_y, new_dist};
                    insertHeap(heap, neighbor);
                }
            }
        }
    }
    free(heap->nodes);
    free(heap);
}

void printNonTunnelingMap() {
    for (int y = 0; y < HEIGHT; y++) {
        for (int x = 0; x < WIDTH; x++) {
            if (x == player_x && y == player_y) {
                printf("@");
            } else if (distance_non_tunnel[y][x] == INFINITY) {
                printf(" ");
            } else {
                printf("%d", distance_non_tunnel[y][x] % 10);
            }
        }
        printf("\n");
    }
}

void printTunnelingMap() {
    for (int y = 0; y < HEIGHT; y++) {
        for (int x = 0; x < WIDTH; x++) {
            if (x == player_x && y == player_y) {
                printf("@");
            } else if (distance_tunnel[y][x] == INFINITY) {
                printf(" ");
            } else {
                printf("%d", distance_tunnel[y][x] % 10);
            }
        }
        printf("\n");
    }
}