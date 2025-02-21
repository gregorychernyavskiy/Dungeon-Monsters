#ifndef PATHFINDING_H
#define PATHFINDING_H

#include "dungeon_generation.h"
#include <stdint.h>

// Structure for a priority queue node
typedef struct {
    int x, y;           // Coordinates in the dungeon
    uint32_t priority;  // Distance value (priority)
} PQNode;

// Priority Queue structure
typedef struct {
    PQNode *nodes;
    int capacity;
    int size;
} PriorityQueue;

PriorityQueue* pq_create(int capacity);
void pq_destroy(PriorityQueue *pq);
void pq_insert(PriorityQueue *pq, int x, int y, uint32_t priority);
PQNode pq_extract_min(PriorityQueue *pq);
void pq_decrease_priority(PriorityQueue *pq, int x, int y, uint32_t new_priority);

// Pathfinding functions
void dijkstra_non_tunneling(uint32_t distances[HEIGHT][WIDTH], int start_x, int start_y);
void dijkstra_tunneling(uint32_t distances[HEIGHT][WIDTH], int start_x, int start_y);

// Utility to print distance maps
void print_distance_map(uint32_t distances[HEIGHT][WIDTH]);

#endif