#include "dungeon_generation.h"
#include "priority_queue.h"

// Distance maps for tunneling and non-tunneling monsters
int tunneling_distance[HEIGHT][WIDTH];
int non_tunneling_distance[HEIGHT][WIDTH];

// Direction arrays for 8-way movement
int dx[8] = {-1, 0, 1, -1, 1, -1, 0, 1};
int dy[8] = {-1, -1, -1, 0, 0, 1, 1, 1};

// Calculate the weight for tunneling
int calculate_tunneling_weight(int hardness) {
    if (hardness == 0) {
        return 1;  // Open space has weight 1
    } else if (hardness >= 1 && hardness <= 254) {
        return 1 + (hardness / 85);  // According to assignment
    } else {
        return INFINITY_DIST;  // Immutable wall
    }
}

// Dijkstra's algorithm for tunneling monsters
void calculate_tunneling_distances() {
    PriorityQueue *pq = pq_init();
    
    // Initialize distances
    for (int y = 0; y < HEIGHT; y++) {
        for (int x = 0; x < WIDTH; x++) {
            tunneling_distance[y][x] = INFINITY_DIST;
        }
    }
    
    // Start from player position
    tunneling_distance[player_y][player_x] = 0;
    pq_enqueue(pq, player_x, player_y, 0);
    
    while (!pq_is_empty(pq)) {
        PQNode *current = pq_dequeue(pq);
        int x = current->x;
        int y = current->y;
        int dist = current->distance;
        
        // Skip if we've found a better path already
        if (dist > tunneling_distance[y][x]) {
            free(current);
            continue;
        }
        
        // Check all neighbors
        for (int i = 0; i < 8; i++) {
            int nx = x + dx[i];
            int ny = y + dy[i];
            
            // Skip if out of bounds
            if (nx < 0 || nx >= WIDTH || ny < 0 || ny >= HEIGHT) {
                continue;
            }
            
            // Calculate the weight based on hardness
            int weight = calculate_tunneling_weight(hardness[ny][nx]);
            
            // Skip immutable walls
            if (weight == INFINITY_DIST) {
                continue;
            }
            
            int new_dist = dist + weight;
            
            // If we found a shorter path
            if (new_dist < tunneling_distance[ny][nx]) {
                tunneling_distance[ny][nx] = new_dist;
                pq_decrease_key(pq, nx, ny, new_dist);
            }
        }
        
        free(current);
    }
    
    pq_free(pq);
}

// Dijkstra's algorithm for non-tunneling monsters
void calculate_non_tunneling_distances() {
    PriorityQueue *pq = pq_init();
    
    // Initialize distances
    for (int y = 0; y < HEIGHT; y++) {
        for (int x = 0; x < WIDTH; x++) {
            non_tunneling_distance[y][x] = INFINITY_DIST;
        }
    }
    
    // Start from player position
    non_tunneling_distance[player_y][player_x] = 0;
    pq_enqueue(pq, player_x, player_y, 0);
    
    while (!pq_is_empty(pq)) {
        PQNode *current = pq_dequeue(pq);
        int x = current->x;
        int y = current->y;
        int dist = current->distance;
        
        // Skip if we've found a better path already
        if (dist > non_tunneling_distance[y][x]) {
            free(current);
            continue;
        }
        
        // Check all neighbors
        for (int i = 0; i < 8; i++) {
            int nx = x + dx[i];
            int ny = y + dy[i];
            
            // Skip if out of bounds
            if (nx < 0 || nx >= WIDTH || ny < 0 || ny >= HEIGHT) {
                continue;
            }
            
            // Skip if not a floor (only path through open spaces)
            if (hardness[ny][nx] != 0) {
                continue;
            }
            
            // Weight is always 1 for non-tunneling
            int new_dist = dist + 1;
            
            // If we found a shorter path
            if (new_dist < non_tunneling_distance[ny][nx]) {
                non_tunneling_distance[ny][nx] = new_dist;
                pq_decrease_key(pq, nx, ny, new_dist);
            }
        }
        
        free(current);
    }
    
    pq_free(pq);
}

// Print the tunneling distance map
void print_tunneling_distance_map() {
    printf("\nTunneling Monster Distance Map:\n");
    for (int y = 0; y < HEIGHT; y++) {
        for (int x = 0; x < WIDTH; x++) {
            if (x == player_x && y == player_y) {
                printf("@");
            } else if (tunneling_distance[y][x] == INFINITY_DIST) {
                printf(" ");
            } else {
                printf("%d", tunneling_distance[y][x] % 10);
            }
        }
        printf("\n");
    }
}

// Print the non-tunneling distance map
void print_non_tunneling_distance_map() {
    printf("\nNon-Tunneling Monster Distance Map:\n");
    for (int y = 0; y < HEIGHT; y++) {
        for (int x = 0; x < WIDTH; x++) {
            if (x == player_x && y == player_y) {
                printf("@");
            } else if (non_tunneling_distance[y][x] == INFINITY_DIST) {
                printf(" ");
            } else {
                printf("%d", non_tunneling_distance[y][x] % 10);
            }
        }
        printf("\n");
    }
}