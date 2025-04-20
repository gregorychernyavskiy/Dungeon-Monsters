#include "dungeon_generation.h"
#include "minheap.h"

void dijkstraNonTunneling(int dist[HEIGHT][WIDTH]) {
    for (int y = 0; y < HEIGHT; y++) {
        for (int x = 0; x < WIDTH; x++) {
            dist[y][x] = HEAP_INFINITY;
        }
    }
    dist[player->y][player->x] = 0;
    MinHeap* heap = createMinHeap(HEIGHT * WIDTH);
    if (!heap) {
        fprintf(stderr, "Failed to create min-heap\n");
        return;
    }
    HeapNode start = {player->x, player->y, 0};
    insertHeap(heap, start);

    int visited[HEIGHT][WIDTH] = {0};
    int dx[] = {-1, 0, 1, -1, 1, -1, 0, 1};
    int dy[] = {-1, -1, -1, 0, 0, 1, 1, 1};

    while (heap->size > 0) {
        HeapNode current = extractMin(heap);
        int x = current.x;
        int y = current.y;
        int curr_dist = current.distance;

        if (visited[y][x]) continue;
        visited[y][x] = 1;
        for (int i = 0; i < 8; i++) {
            int nx = x + dx[i];
            int ny = y + dy[i];
            if (nx < 0 || nx >= WIDTH || ny < 0 || ny >= HEIGHT || visited[ny][nx] || hardness[ny][nx] != 0) continue;

            int new_dist = curr_dist + 1;
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

void dijkstraTunneling(int dist[HEIGHT][WIDTH]) {
    for (int y = 0; y < HEIGHT; y++) {
        for (int x = 0; x < WIDTH; x++) {
            dist[y][x] = HEAP_INFINITY;
        }
    }
    dist[player->y][player->x] = 0;
    MinHeap* heap = createMinHeap(HEIGHT * WIDTH);
    if (!heap) {
        fprintf(stderr, "Failed to create min-heap\n");
        return;
    }
    HeapNode start = {player->x, player->y, 0};
    insertHeap(heap, start);

    int visited[HEIGHT][WIDTH] = {0};
    int dx[] = {-1, 0, 1, -1, 1, -1, 0, 1};
    int dy[] = {-1, -1, -1, 0, 0, 1, 1, 1};

    while (heap->size > 0) {
        HeapNode current = extractMin(heap);
        int x = current.x;
        int y = current.y;
        int curr_dist = current.distance;

        if (visited[y][x]) continue;
        visited[y][x] = 1;
        for (int i = 0; i < 8; i++) {
            int nx = x + dx[i];
            int ny = y + dy[i];
            if (nx < 0 || nx >= WIDTH || ny < 0 || ny >= HEIGHT || visited[ny][nx] || hardness[ny][nx] == 255) continue;

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

void printNonTunnelingMap() {
    int dist[HEIGHT][WIDTH];
    dijkstraNonTunneling(dist);
    for (int y = 0; y < HEIGHT; y++) {
        for (int x = 0; x < WIDTH; x++) {
            if (x == player->x && y == player->y) {
                printf("@");
            } else if (dist[y][x] == HEAP_INFINITY) {
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
            if (x == player->x && y == player->y) {
                printf("@");
            } else if (dist[y][x] == HEAP_INFINITY) {
                printf("%c", dungeon[y][x]);
            } else {
                printf("%d", dist[y][x] % 10);
            }
        }
        printf("\n");
    }
}