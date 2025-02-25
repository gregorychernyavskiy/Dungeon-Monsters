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

void dijkstra(uint32_t distances[HEIGHT][WIDTH], int start_x, int start_y) {
    // Initialize distances
    for (int y = 0; y < HEIGHT; y++) {
        for (int x = 0; x < WIDTH; x++) {
            distances[y][x] = INFINITY;
        }
    }
    distances[start_y][start_x] = 0;

    MinHeap* heap = createMinHeap(HEIGHT * WIDTH);
    heapPush(heap, start_x, start_y, 0);

    int dx[] = {-1, 1, 0, 0};
    int dy[] = {0, 0, -1, 1};

    while (heap->size > 0) {
        HeapNode current = heapPop(heap);
        int x = current.x;
        int y = current.y;
        uint32_t cost = current.cost;

        if (cost > distances[y][x]) {
            continue;
        }

        for (int i = 0; i < 4; i++) {
            int new_x = x + dx[i];
            int new_y = y + dy[i];

            if (new_x >= 0 && new_x < WIDTH && new_y >= 0 && new_y < HEIGHT) {
                if (hardness[new_y][new_x] == 255) continue; // Impassable

                uint32_t new_cost = cost + (hardness[new_y][new_x] == 0 ? 1 : hardness[new_y][new_x]);
                if (new_cost < distances[new_y][new_x]) {
                    distances[new_y][new_x] = new_cost;
                    heapPush(heap, new_x, new_y, new_cost);
                }
            }
        }
    }

    destroyMinHeap(heap);
}