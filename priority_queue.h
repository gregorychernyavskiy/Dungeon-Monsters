#ifndef PRIORITY_QUEUE_H
#define PRIORITY_QUEUE_H

#include <stdio.h>
#include <stdlib.h>
#include <limits.h>

#define INFINITY_DIST INT_MAX

// Node structure for the priority queue
typedef struct PQNode {
    int x;
    int y;
    int distance;
    struct PQNode *next;
} PQNode;

// Priority Queue structure
typedef struct {
    PQNode *head;
    int size;
} PriorityQueue;

// Priority queue operations
PriorityQueue* pq_init();
int pq_is_empty(PriorityQueue *pq);
void pq_enqueue(PriorityQueue *pq, int x, int y, int distance);
PQNode* pq_dequeue(PriorityQueue *pq);
void pq_decrease_key(PriorityQueue *pq, int x, int y, int new_distance);
int pq_contains(PriorityQueue *pq, int x, int y);
void pq_free(PriorityQueue *pq);

#endif