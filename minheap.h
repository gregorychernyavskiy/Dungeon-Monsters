// minheap.h
#ifndef MINHEAP_H
#define MINHEAP_H

#include <stdlib.h>
#include <limits.h>

#define HEAP_INFINITY INT_MAX

typedef struct {
    int x;
    int y;
    int distance;
} HeapNode;

typedef struct {
    HeapNode* nodes;
    int size;
    int capacity;
} MinHeap;

MinHeap* createMinHeap(int capacity);
void heapify(MinHeap* heap, int idx);
void insertHeap(MinHeap* heap, HeapNode node);
HeapNode extractMin(MinHeap* heap);
void decreasePriority(MinHeap* heap, int x, int y, int newDistance);

#endif