#include "dungeon_generation.h"
#include "minheap.h"

static int isValid(int x, int y) {
    return (x >= 0 && x < WIDTH && y >= 0 && y < HEIGHT);
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