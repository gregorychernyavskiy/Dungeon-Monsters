#include "minheap.h"

MinHeap* createMinHeap(int capacity) {
    MinHeap* heap = (MinHeap*)malloc(sizeof(MinHeap));
    if (!heap) return nullptr;
    heap->size = 0;
    heap->capacity = capacity;
    heap->nodes = (HeapNode*)malloc(capacity * sizeof(HeapNode));
    if (!heap->nodes) {
        free(heap);
        return nullptr;
    }
    return heap;
}

void heapify(MinHeap* heap, int idx) {
    int smallest = idx;
    int left = 2 * idx + 1;
    int right = 2 * idx + 2;

    if (left < heap->size && heap->nodes[left].distance < heap->nodes[smallest].distance) {
        smallest = left;
    }
    if (right < heap->size && heap->nodes[right].distance < heap->nodes[smallest].distance) {
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
    if (heap->size == heap->capacity) return;

    heap->nodes[heap->size] = node;
    int i = heap->size++;
    while (i != 0 && heap->nodes[(i-1)/2].distance > heap->nodes[i].distance) {
        HeapNode temp = heap->nodes[i];
        heap->nodes[i] = heap->nodes[(i-1)/2];
        heap->nodes[(i-1)/2] = temp;
        i = (i-1)/2;
    }
}

HeapNode extractMin(MinHeap* heap) {
    if (heap->size <= 0) return {-1, -1, INFINITY};
    if (heap->size == 1) return heap->nodes[--heap->size];

    HeapNode root = heap->nodes[0];
    heap->nodes[0] = heap->nodes[--heap->size];
    heapify(heap, 0);
    return root;
}

void decreasePriority(MinHeap* heap, int x, int y, int newDistance) {
    int i;
    for (i = 0; i < heap->size; i++) {
        if (heap->nodes[i].x == x && heap->nodes[i].y == y) break;
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