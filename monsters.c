#include "dungeon_generation.h"

Monster *monsters = NULL;

static char getMonsterSymbol(int intelligent, int telepathic, int tunneling, int erratic) {
    int value = (erratic << 3) | (tunneling << 2) | (telepathic << 1) | intelligent;
    if (value < 10) return '0' + value;
    return 'a' + (value - 10);
}

void spawnMonsters(int count) {
    monsters = malloc(count * sizeof(Monster));
    if (!monsters) {
        perror("Failed to allocate monsters");
        exit(EXIT_FAILURE);
    }
    num_monsters = count;

    for (int i = 0; i < count; i++) {
        int placed = 0;
        while (!placed) {
            int room_idx = rand() % num_rooms;
            int x = rooms[room_idx].x + rand() % rooms[room_idx].width;
            int y = rooms[room_idx].y + rand() % rooms[room_idx].height;
            if (dungeon[y][x] == '.' && !(x == pc.x && y == pc.y)) {
                monsters[i].x = x;
                monsters[i].y = y;
                monsters[i].speed = MIN_MONSTER_SPEED + rand() % (MAX_MONSTER_SPEED - MIN_MONSTER_SPEED + 1);
                monsters[i].intelligent = rand() % 2;
                monsters[i].telepathic = rand() % 2;
                monsters[i].tunneling = rand() % 2;
                monsters[i].erratic = rand() % 2;
                monsters[i].symbol = getMonsterSymbol(monsters[i].intelligent, monsters[i].telepathic,
                                                      monsters[i].tunneling, monsters[i].erratic);
                monsters[i].alive = 1;
                monsters[i].last_seen_x = -1;
                monsters[i].last_seen_y = -1;
                dungeon[y][x] = monsters[i].symbol;
                placed = 1;
            }
        }
    }
}

int hasLineOfSight(int x1, int y1, int x2, int y2) {
    int dx = abs(x2 - x1);
    int dy = abs(y2 - y1);
    int sx = (x1 < x2) ? 1 : -1;
    int sy = (y1 < y2) ? 1 : -1;
    int err = dx - dy;

    int x = x1;
    int y = y1;

    while (x != x2 || y != y2) {
        if (hardness[y][x] > 0 && dungeon[y][x] != '@' && dungeon[y][x] != '#') return 0;
        int e2 = 2 * err;
        if (e2 > -dy) {
            err -= dy;
            x += sx;
        }
        if (e2 < dx) {
            err += dx;
            y += sy;
        }
    }
    return 1;
}

void moveMonster(int index) {
    if (!monsters[index].alive) return;

    int new_x = monsters[index].x;
    int new_y = monsters[index].y;

    // Erratic behavior
    if (monsters[index].erratic && rand() % 2) {
        int dx[] = {-1, 0, 1, -1, 1, -1, 0, 1, 0};
        int dy[] = {-1, -1, -1, 0, 0, 1, 1, 1, 0};
        int dir = rand() % 9;
        new_x = monsters[index].x + dx[dir];
        new_y = monsters[index].y + dy[dir];
        if (new_x < 0 || new_x >= WIDTH || new_y < 0 || new_y >= HEIGHT || hardness[new_y][new_x] == 255 ||
            (!monsters[index].tunneling && hardness[new_y][new_x] > 0)) {
            return; // Invalid move
        }
    } else {
        // Determine target
        int target_x, target_y;
        if (monsters[index].telepathic || hasLineOfSight(monsters[index].x, monsters[index].y, pc.x, pc.y)) {
            target_x = pc.x;
            target_y = pc.y;
            if (monsters[index].intelligent) {
                monsters[index].last_seen_x = pc.x;
                monsters[index].last_seen_y = pc.y;
            }
        } else if (monsters[index].intelligent && monsters[index].last_seen_x != -1) {
            target_x = monsters[index].last_seen_x;
            target_y = monsters[index].last_seen_y;
        } else {
            return; // No movement if no target
        }

        // Use appropriate distance map
        int (*dist)[WIDTH] = monsters[index].tunneling ? distance_tunnel : distance_non_tunnel;
        int dx[] = {-1, 0, 1, -1, 1, -1, 0, 1};
        int dy[] = {-1, -1, -1, 0, 0, 1, 1, 1};
        int min_dist = dist[monsters[index].y][monsters[index].x];
        for (int i = 0; i < 8; i++) {
            int nx = monsters[index].x + dx[i];
            int ny = monsters[index].y + dy[i];
            if (nx >= 0 && nx < WIDTH && ny >= 0 && ny < HEIGHT && dist[ny][nx] < min_dist &&
                (monsters[index].tunneling || hardness[ny][nx] == 0)) {
                min_dist = dist[ny][nx];
                new_x = nx;
                new_y = ny;
            }
        }
    }

    // Handle tunneling
    if (monsters[index].tunneling && hardness[new_y][new_x] > 0) {
        hardness[new_y][new_x] = (hardness[new_y][new_x] > 85) ? hardness[new_y][new_x] - 85 : 0;
        if (hardness[new_y][new_x] == 0) dungeon[new_y][new_x] = '#';
        updateDistanceMaps();
        return;
    }

    // Check for collision with PC
    if (new_x == pc.x && new_y == pc.y) {
        pc.alive = 0;
        printf("Monster %c killed the player at (%d, %d)\n", monsters[index].symbol, new_x, new_y);
        return;
    }

    // Move monster
    dungeon[monsters[index].y][monsters[index].x] = (hardness[monsters[index].y][monsters[index].x] == 0) ? '.' : '#';
    monsters[index].x = new_x;
    monsters[index].y = new_y;
    dungeon[new_y][new_x] = monsters[index].symbol;
}

void updateDistanceMaps() {
    dijkstraNonTunneling(distance_non_tunnel);
    dijkstraTunneling(distance_tunnel);
}

void freeMonsters() {
    if (monsters) free(monsters);
    monsters = NULL;
}