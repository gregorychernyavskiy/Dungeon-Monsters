#include "dungeon_generation.h"
#include "minheap.h"

static int isValid(int x, int y) {
    return (x >= 0 && x < WIDTH && y >= 0 && y < HEIGHT);
}

void dijkstraNonTunneling(int dist[HEIGHT][WIDTH]) {
    
    for (int y = 0; y < HEIGHT; y++) {
        for (int x = 0; x < WIDTH; x++) {
            dist[y][x] = INFINITY;
        }
    }
    dist[player_y][player_x] = 0;

    
    int queue[HEIGHT * WIDTH][3];
    int q_size = 0;
    int visited[HEIGHT][WIDTH] = {0};

    queue[q_size][0] = player_x;
    queue[q_size][1] = player_y;
    queue[q_size][2] = 0;
    q_size++;

    int dx[] = {-1, 0, 1, -1, 1, -1, 0, 1};
    int dy[] = {-1, -1, -1, 0, 0, 1, 1, 1};

    while (q_size > 0) {
        
        int x = queue[0][0];
        int y = queue[0][1];
        int curr_dist = queue[0][2];

        for (int i = 0; i < q_size - 1; i++) {
            queue[i][0] = queue[i + 1][0];
            queue[i][1] = queue[i + 1][1];
            queue[i][2] = queue[i + 1][2];
        }
        q_size--;

        if (visited[y][x]) continue;
        visited[y][x] = 1;

        for (int i = 0; i < 8; i++) {
            int nx = x + dx[i];
            int ny = y + dy[i];

            if (nx < 0 || nx >= WIDTH || ny < 0 || ny >= HEIGHT) continue;
            if (visited[ny][nx]) continue;
            if (hardness[ny][nx] != 0) continue;

            int new_dist = curr_dist + 1;
            if (new_dist < dist[ny][nx]) {
                dist[ny][nx] = new_dist;
                queue[q_size][0] = nx;
                queue[q_size][1] = ny;
                queue[q_size][2] = new_dist;
                q_size++;
            }
        }
    }
}

void printNonTunnelingMap() {
    int dist[HEIGHT][WIDTH];
    dijkstraNonTunneling(dist);

    for (int y = 0; y < HEIGHT; y++) {
        for (int x = 0; x < WIDTH; x++) {
            if (x == player_x && y == player_y) {
                printf("@");
            } else if (dist[y][x] == INFINITY) {
                printf("%c", dungeon[y][x]);
            } else {
                printf("%d", dist[y][x] % 10);
            }
        }
        printf("\n");
    }
}











void dijkstraTunneling(int dist[HEIGHT][WIDTH]) {
    for (int y = 0; y < HEIGHT; y++) {
        for (int x = 0; x < WIDTH; x++) {
            dist[y][x] = INFINITY;
        }
    }
    dist[player_y][player_x] = 0;

    int visited[HEIGHT][WIDTH] = {0};
    int queue[HEIGHT * WIDTH][3];
    int q_size = 0;

    queue[q_size][0] = player_x;
    queue[q_size][1] = player_y;
    queue[q_size][2] = 0;
    q_size++;
    int dx[] = {-1, 0, 1, -1, 1, -1, 0, 1};
    int dy[] = {-1, -1, -1, 0, 0, 1, 1, 1};

    while (q_size > 0) {
        int min_idx = 0;
        for (int i = 1; i < q_size; i++) {
            if (queue[i][2] < queue[min_idx][2]) {
                min_idx = i;
            }
        }

        int x = queue[min_idx][0];
        int y = queue[min_idx][1];
        int curr_dist = queue[min_idx][2];

        queue[min_idx][0] = queue[q_size - 1][0];
        queue[min_idx][1] = queue[q_size - 1][1];
        queue[min_idx][2] = queue[q_size - 1][2];
        q_size--;

        if (visited[y][x]) continue;
        visited[y][x] = 1;

        for (int i = 0; i < 8; i++) {
            int nx = x + dx[i];
            int ny = y + dy[i];

            if (nx < 0 || nx >= WIDTH || ny < 0 || ny >= HEIGHT) continue;
            if (visited[ny][nx]) continue;

            int weight;
            if (hardness[ny][nx] == 0) {
                weight = 1;
            } else if (hardness[ny][nx] == 255) {
                continue;
            } else {
                weight = 1 + hardness[ny][nx] / 85;
            }

            int new_dist = curr_dist + weight;
            if (new_dist < dist[ny][nx]) {
                dist[ny][nx] = new_dist;
                
                queue[q_size][0] = nx;
                queue[q_size][1] = ny;
                queue[q_size][2] = new_dist;
                q_size++;
            }
        }
    }
}
void printTunnelingMap() {
    int dist[HEIGHT][WIDTH];
    dijkstraTunneling(dist);

    for (int y = 0; y < HEIGHT; y++) {
        for (int x = 0; x < WIDTH; x++) {
            if (x == player_x && y == player_y) {
                printf("@");
            } else if (dist[y][x] == INFINITY) {
                printf("%c", dungeon[y][x]);
            } else {
                printf("%d", dist[y][x] % 10);
            }
        }
        printf("\n");
    }
}