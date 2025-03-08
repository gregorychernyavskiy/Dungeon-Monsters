#include "dungeon_generation.h"
#include "minheap.h"


void dijkstraNonTunneling(int dist[HEIGHT][WIDTH]) {
    // Initialize distances to infinity
    for (int y = 0; y < HEIGHT; y++) {
        for (int x = 0; x < WIDTH; x++) {
            dist[y][x] = INFINITY;
        }
    }
    dist[player_y][player_x] = 0;
    // Create min-heap for priority queue
    MinHeap* heap = createMinHeap(HEIGHT * WIDTH);
    if (!heap) {
        fprintf(stderr, "Failed to create min-heap\n");
        return;
    }
    // Insert starting node (player position)
    HeapNode start = {player_x, player_y, 0};
    insertHeap(heap, start);

    int visited[HEIGHT][WIDTH] = {0};
    int dx[] = {-1, 0, 1, -1, 1, -1, 0, 1}; // 8-way connectivity
    int dy[] = {-1, -1, -1, 0, 0, 1, 1, 1};

    while (heap->size > 0) {
        HeapNode current = extractMin(heap);
        int x = current.x;
        int y = current.y;
        int curr_dist = current.distance;

        if (visited[y][x]) continue;
        visited[y][x] = 1;
        // Explore 8 neighbors
        for (int i = 0; i < 8; i++) {
            int nx = x + dx[i];
            int ny = y + dy[i];

            if (nx < 0 || nx >= WIDTH || ny < 0 || ny >= HEIGHT) continue;
            if (visited[ny][nx]) continue;
            if (hardness[ny][nx] != 0) continue; // Non-tunneling: only move through hardness 0

            int new_dist = curr_dist + 1; // Weight is always 1 for floor
            if (new_dist < dist[ny][nx]) {
                dist[ny][nx] = new_dist;
                HeapNode next = {nx, ny, new_dist};
                insertHeap(heap, next);
                // No need for decreasePriority here since we only insert unvisited nodes
            }
        }
    }
    // Free heap memory
    free(heap->nodes);
    free(heap);
}



void dijkstraTunneling(int dist[HEIGHT][WIDTH]) {
    // Initialize distances to infinity
    for (int y = 0; y < HEIGHT; y++) {
        for (int x = 0; x < WIDTH; x++) {
            dist[y][x] = INFINITY;
        }
    }
    dist[player_y][player_x] = 0;
    // Create min-heap for priority queue
    MinHeap* heap = createMinHeap(HEIGHT * WIDTH);
    if (!heap) {
        fprintf(stderr, "Failed to create min-heap\n");
        return;
    }
    // Insert starting node (player position)
    HeapNode start = {player_x, player_y, 0};
    insertHeap(heap, start);

    int visited[HEIGHT][WIDTH] = {0};
    int dx[] = {-1, 0, 1, -1, 1, -1, 0, 1}; // 8-way connectivity
    int dy[] = {-1, -1, -1, 0, 0, 1, 1, 1};

    while (heap->size > 0) {
        HeapNode current = extractMin(heap);
        int x = current.x;
        int y = current.y;
        int curr_dist = current.distance;

        if (visited[y][x]) continue;
        visited[y][x] = 1;

        // Explore 8 neighbors
        for (int i = 0; i < 8; i++) {
            int nx = x + dx[i];
            int ny = y + dy[i];

            if (nx < 0 || nx >= WIDTH || ny < 0 || ny >= HEIGHT) continue;
            if (visited[ny][nx]) continue;
            if (hardness[ny][nx] == 255) continue; // Infinite weight for hardness 255

            // Calculate weight based on hardness
            int weight = (hardness[ny][nx] == 0) ? 1 : 1 + hardness[ny][nx] / 85;
            int new_dist = curr_dist + weight;

            if (new_dist < dist[ny][nx]) {
                dist[ny][nx] = new_dist;
                HeapNode next = {nx, ny, new_dist};
                insertHeap(heap, next);
                // Note: If a node is already in the heap with a higher distance,
                // we could use decreasePriority, but for simplicity, we allow duplicates
            }
        }
    }
    // Free heap memory
    free(heap->nodes);
    free(heap);
}





void printNonTunnelingMap() {
    int dist[HEIGHT][WIDTH];
    dijkstraNonTunneling(dist);

    for(int y = 0; y < HEIGHT; y++) {
        for(int x = 0; x < WIDTH; x++) {
            if(x == player_x && y == player_y) {
                printf("@");
            } else if(dist[y][x] == INFINITY) {
                printf("%c", dungeon[y][x]);
            } else{
                printf("%d", dist[y][x] % 10);
            }
        }
        printf("\n");
    }
}

void printTunnelingMap() {
    int dist[HEIGHT][WIDTH];
    dijkstraTunneling(dist);

    for(int y = 0; y < HEIGHT; y++) {
        for(int x = 0; x < WIDTH; x++) {
            if(x == player_x && y == player_y) {
                printf("@");
            } else if(dist[y][x] == INFINITY) {
                printf("%c", dungeon[y][x]);
            } else{
                printf("%d", dist[y][x] % 10);
            }
        }
        printf("\n");
    }
}