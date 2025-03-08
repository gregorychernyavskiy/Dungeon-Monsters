#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <stdio.h>
#include <ctype.h>
#include <unistd.h>
#include "dungeon.h"
#include "saveLoad.h"
#include "pathFinding.h"
#include "fibonacciHeap.h"

int main(int argc, char *argv[]) {
    int hardnessBeforeFlag = 0;
    int hardnessAfterFlag = 0;
    int saveFlag = 0;
    int loadFlag = 0;
    int monTypeFlag = 0;

    srand(time(NULL));

    char filename[256];
    char monType = '0';
    int numMonsters = rand() % 14 + 7;

    // Argument parsing (unchanged)
    for (int i = 1; i < argc; i++) {
        if (!strcmp(argv[i], "-hb")) {
            hardnessBeforeFlag = 1;
        } else if (!strcmp(argv[i], "-ha")) {
            hardnessAfterFlag = 1;
        } else if (!strcmp(argv[i], "--save")) {
            if (i < argc - 1 && argv[i + 1][0] != '-') {
                strncpy(filename, argv[i + 1], sizeof(filename) - 1);
                filename[sizeof(filename) - 1] = '\0';
                saveFlag = 1;
                i++;
            } else {
                fprintf(stderr, "Error: Argument '--save' requires a file name\n");
                return 1;
            }
        } else if (!strcmp(argv[i], "--load")) {
            if (i < argc - 1 && argv[i + 1][0] != '-') {
                strncpy(filename, argv[i + 1], sizeof(filename) - 1);
                filename[sizeof(filename) - 1] = '\0';
                loadFlag = 1;
                i++;
            } else {
                fprintf(stderr, "Error: Argument '--load' requires a file name\n");
                return 1;
            }
        } else if (!strcmp(argv[i], "--nummon")) {
            if (i < argc - 1 && atoi(argv[i + 1]) >= 0) {
                numMonsters = atoi(argv[i + 1]);
                i++;
            } else {
                fprintf(stderr, "Error: Argument '--nummon' requires a positive integer\n");
                return 1;
            }
        } else if (!strcmp(argv[i], "--montype")) {
            if (i < argc - 1 && strlen(argv[i + 1]) == 1) {
                monType = argv[i + 1][0];
                numMonsters = 1;
                monTypeFlag = 1;
                i++;
            } else {
                fprintf(stderr, "Error: Argument '--montype' requires a single character\n");
                return 1;
            }
        } else {
            fprintf(stderr, "Error: Unrecognized argument '%s'\n", argv[i]);
            return 1;
        }
    }

    // Dungeon initialization (unchanged)
    if (loadFlag) {
        if (hardnessBeforeFlag) {
            fprintf(stderr, "Error: Argument '--load' and '-hb' cannot be used together\n");
            return 1;
        }
        loadDungeon(filename);
        if (monTypeFlag) {
            if (populateDungeonWithMonType(monType)) return 1;
        } else {
            if (populateDungeon(numMonsters)) return 1;
        }
    } else {
        initDungeon();
        if (hardnessBeforeFlag) printHardness();
        if (monTypeFlag) {
            if (fillDungeonWithMonType(monType)) return 1;
        } else {
            if (fillDungeon(numMonsters)) return 1;
        }
    }

    if (hardnessAfterFlag) printHardness();
    if (saveFlag && !loadFlag) saveDungeon(filename);

    printTunnelingDistances();
    printNonTunnelingDistances();

    // Game loop with stationary player and attacking monsters
    int time = 0;
    int monstersAlive = numMonsters;
    FibNode *nodes[MAX_HEIGHT][MAX_WIDTH] = {NULL};
    FibHeap *heap = createFibHeap();
    if (!heap) {
        cleanup(numMonsters);
        return 1;
    }

    // Add monsters to the heap (no player movement)
    for (int i = 0; i < MAX_HEIGHT; i++) {
        for (int j = 0; j < MAX_WIDTH; j++) {
            if (monsterAt[i][j]) {
                nodes[i][j] = insert(heap, 0, (Pos){j, i}); // Initial time 0
                if (!nodes[i][j]) {
                    destroyFibHeap(heap);
                    cleanup(numMonsters);
                    return 1;
                }
            }
        }
    }

    printDungeon(); // Initial dungeon state

    while (heap->min) {
        FibNode *node = extractMin(heap);
        if (!node) {
            destroyFibHeap(heap);
            cleanup(numMonsters);
            return 1;
        }
        time = node->key;
        Mon *mon = monsterAt[node->pos.y][node->pos.x];
        if (!mon) {
            free(node);
            continue;
        }

        int x = mon->pos.x;
        int y = mon->pos.y;
        int newX = x, newY = y;

        // Monster movement logic from original code, adapted for stationary player
        int isIntelligent = mon->intelligent;
        int isTunneling = mon->tunneling;
        int isTelepathic = mon->telepathic;
        int isErratic = mon->erratic;

        int directions[9][2] = {
            {-1, 1}, {0, 1}, {1, 1},
            {-1, 0}, {0, 0}, {1, 0},
            {-1, -1}, {0, -1}, {1, -1}
        };

        if (isErratic && rand() % 2) {
            int dir = rand() % 9;
            newX = x + directions[dir][0];
            newY = y + directions[dir][1];
        } else if (isTelepathic || (abs(x - player.x) <= 1 && abs(y - player.y) <= 1)) {
            if (isIntelligent) {
                int minDist = UNREACHABLE;
                int dir = 4; // Default: stay still
                for (int i = 0; i < 9; i++) {
                    int nx = x + directions[i][0];
                    int ny = y + directions[i][1];
                    if (nx >= 0 && nx < MAX_WIDTH && ny >= 0 && ny < MAX_HEIGHT) {
                        int dist = isTunneling ? dungeon[ny][nx].tunnelingDist : dungeon[ny][nx].nonTunnelingDist;
                        if (dist < minDist && dist != UNREACHABLE) {
                            minDist = dist;
                            dir = i;
                        }
                    }
                }
                newX = x + directions[dir][0];
                newY = y + directions[dir][1];
            } else {
                int xDir = (player.x > x) ? 1 : (player.x < x) ? -1 : 0;
                int yDir = (player.y > y) ? 1 : (player.y < y) ? -1 : 0;
                newX = x + xDir;
                newY = y + yDir;
            }
        }

        // Handle movement and combat
        if (newX >= 0 && newX < MAX_WIDTH && newY >= 0 && newY < MAX_HEIGHT) {
            if (isTunneling && dungeon[newY][newX].type == ROCK && dungeon[newY][newX].hardness < 255) {
                if (dungeon[newY][newX].hardness > 85) {
                    dungeon[newY][newX].hardness -= 85;
                    nodes[y][x] = insert(heap, time + 1000 / mon->speed, mon->pos);
                } else {
                    dungeon[newY][newX].hardness = 0;
                    dungeon[newY][newX].type = CORRIDOR;
                    mon->pos.x = newX;
                    mon->pos.y = newY;
                    monsterAt[y][x] = NULL;
                    monsterAt[newY][newX] = mon;
                    nodes[y][x] = NULL;
                    nodes[newY][newX] = insert(heap, time + 1000 / mon->speed, mon->pos);
                    generateDistances();
                }
            } else if (dungeon[newY][newX].hardness == 0) {
                if (newX == player.x && newY == player.y) {
                    mon->pos.x = newX;
                    mon->pos.y = newY;
                    monsterAt[y][x] = NULL;
                    monsterAt[newY][newX] = mon;
                    nodes[y][x] = NULL;
                    printDungeon();
                    printf("Player killed by monster, gg\n");
                    destroyFibHeap(heap);
                    cleanup(numMonsters);
                    free(node);
                    return 1;
                } else if (!monsterAt[newY][newX]) {
                    mon->pos.x = newX;
                    mon->pos.y = newY;
                    monsterAt[y][x] = NULL;
                    monsterAt[newY][newX] = mon;
                    nodes[y][x] = NULL;
                    nodes[newY][newX] = insert(heap, time + 1000 / mon->speed, mon->pos);
                }
            } else {
                nodes[y][x] = insert(heap, time + 1000 / mon->speed, mon->pos);
            }
        } else {
            nodes[y][x] = insert(heap, time + 1000 / mon->speed, mon->pos);
        }

        free(node);
        usleep(200000); // Pause for 0.2 seconds
        printDungeon();
    }

    // Unreachable: all monsters dead (unlikely with stationary player)
    printf("Player killed all monsters\n");
    destroyFibHeap(heap);
    cleanup(numMonsters);
    return 0;
}