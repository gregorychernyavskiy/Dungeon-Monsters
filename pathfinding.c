#include "pathfinding.h"
#include <stdlib.h>
#include <limits.h>

#define INFINITY UINT32_MAX

PriorityQueue* pq_create(int capacity) {
    PriorityQueue *pq = malloc(sizeof(PriorityQueue));
    pq->nodes = malloc(sizeof(PQNode) * capacity);
    pq->capacity = capacity;
    pq->size = 0;
    return pq;
}

void pq_destroy(PriorityQueue *pq) {
    free(pq->nodes);
    free(pq);
}

void pq_insert(PriorityQueue *pq, int x, int y, uint32_t priority) {
    if (pq->size >= pq->capacity) return;
    int i = pq->size++;
    pq->nodes[i].x = x;
    pq->nodes[i].y = y;
    pq->nodes[i].priority = priority;
    while (i > 0 && pq->nodes[(i-1)/2].priority > pq->nodes[i].priority) {
        PQNode temp = pq->nodes[i];
        pq->nodes[i] = pq->nodes[(i-1)/2];
        pq->nodes[(i-1)/2] = temp;
        i = (i-1)/2;
    }
}

PQNode pq_extract_min(PriorityQueue *pq) {
    if (pq->size == 0) {
        PQNode empty = {-1, -1, INFINITY};
        return empty;
    }
    PQNode min = pq->nodes[0];
    pq->nodes[0] = pq->nodes[--pq->size];
    int i = 0;
    while (2*i + 1 < pq->size) {
        int child = 2*i + 1;
        if (child + 1 < pq->size && pq->nodes[child + 1].priority < pq->nodes[child].priority) {
            child++;
        }
        if (pq->nodes[i].priority <= pq->nodes[child].priority) break;
        PQNode temp = pq->nodes[i];
        pq->nodes[i] = pq->nodes[child];
        pq->nodes[child] = temp;
        i = child;
    }
    return min;
}

void pq_decrease_priority(PriorityQueue *pq, int x, int y, uint32_t new_priority) {
    for (int i = 0; i < pq->size; i++) {
        if (pq->nodes[i].x == x && pq->nodes[i].y == y && pq->nodes[i].priority > new_priority) {
            pq->nodes[i].priority = new_priority;
            while (i > 0 && pq->nodes[(i-1)/2].priority > pq->nodes[i].priority) {
                PQNode temp = pq->nodes[i];
                pq->nodes[i] = pq->nodes[(i-1)/2];
                pq->nodes[(i-1)/2] = temp;
                i = (i-1)/2;
            }
            return;
        }
    }
}

void dijkstra_non_tunneling(uint32_t distances[HEIGHT][WIDTH], int start_x, int start_y) {
    for (int y = 0; y < HEIGHT; y++) {
        for (int x = 0; x < WIDTH; x++) {
            distances[y][x] = INFINITY;
        }
    }
    distances[start_y][start_x] = 0;

    PriorityQueue *pq = pq_create(WIDTH * HEIGHT);
    pq_insert(pq, start_x, start_y, 0);

    int dx[] = {-1, -1, -1, 0, 0, 1, 1, 1};
    int dy[] = {-1, 0, 1, -1, 1, -1, 0, 1};

    while (pq->size > 0) {
        PQNode current = pq_extract_min(pq);
        int x = current.x;
        int y = current.y;
        uint32_t dist = current.priority;

        if (dist > distances[y][x]) continue;

        for (int i = 0; i < 8; i++) {
            int nx = x + dx[i];
            int ny = y + dy[i];
            if (nx >= 0 && nx < WIDTH && ny >= 0 && ny < HEIGHT && hardness[ny][nx] == 0) {
                uint32_t new_dist = dist + 1;
                if (new_dist < distances[ny][nx]) {
                    distances[ny][nx] = new_dist;
                    pq_insert(pq, nx, ny, new_dist);
                }
            }
        }
    }
    pq_destroy(pq);
}

void dijkstra_tunneling(uint32_t distances[HEIGHT][WIDTH], int start_x, int start_y) {
    for (int y = 0; y < HEIGHT; y++) {
        for (int x = 0; x < WIDTH; x++) {
            distances[y][x] = INFINITY;
        }
    }
    distances[start_y][start_x] = 0;

    PriorityQueue *pq = pq_create(WIDTH * HEIGHT);
    pq_insert(pq, start_x, start_y, 0);

    int dx[] = {-1, -1, -1, 0, 0, 1, 1, 1};
    int dy[] = {-1, 0, 1, -1, 1, -1, 0, 1};

    while (pq->size > 0) {
        PQNode current = pq_extract_min(pq);
        int x = current.x;
        int y = current.y;
        uint32_t dist = current.priority;

        if (dist > distances[y][x]) continue;

        for (int i = 0; i < 8; i++) {
            int nx = x + dx[i];
            int ny = y + dy[i];
            if (nx >= 0 && nx < WIDTH && ny >= 0 && ny < HEIGHT && hardness[ny][nx] != 255) {
                uint32_t weight = (hardness[ny][nx] == 0) ? 1 : 1 + (hardness[ny][nx] / 85);
                uint32_t new_dist = dist + weight;
                if (new_dist < distances[ny][nx]) {
                    distances[ny][nx] = new_dist;
                    pq_insert(pq, nx, ny, new_dist);
                }
            }
        }
    }
    pq_destroy(pq);
}

void print_distance_map(uint32_t distances[HEIGHT][WIDTH]) {
    for (int y = 0; y < HEIGHT; y++) {
        for (int x = 0; x < WIDTH; x++) {
            if (x == player_x && y == player_y) {
                printf("@");
            } else if (distances[y][x] == INFINITY) {
                printf(" ");
            } else {
                printf("%d", distances[y][x] % 10);
            }
        }
        printf("\n");
    }
}