#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <ctype.h>
#include "dungeon_generation.h"
#include "minheap.h"

int main(int argc, char *argv[]) {
    srand(time(NULL));
    int load = 0, save = 0, numMonsters = 0;
    char *saveFileName = NULL;
    char *loadFileName = NULL;

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--save") == 0) {
            save = 1;
            if (i + 1 < argc) {
                saveFileName = argv[i + 1];
                i++;
            } else {
                printf("Error: Missing filename for --save\n");
                return 1;
            }
        } else if (strcmp(argv[i], "--load") == 0) {
            load = 1;
            if (i + 1 < argc) {
                loadFileName = argv[i + 1];
                i++;
            } else {
                printf("Error: Missing filename for --load\n");
                return 1;
            }
        } else {
            char *endptr;
            long num = strtol(argv[i], &endptr, 10);
            if (*endptr == '\0' && num >= 0) {
                numMonsters = (int)num;
            }
        }
    }

    if (load) {
        if (loadFileName) {
            loadDungeon(loadFileName);
            printf("Dungeon:\n");
            printDungeon();
            printf("\nHardness:\n");
            printHardness();
            printf("\nNon-Tunneling Distance Map:\n");
            printNonTunnelingMap();
            printf("\nTunneling Distance Map:\n");
            printTunnelingMap();
            return 0;
        } else {
            printf("Error: No file path specified for loading!\n");
            return 1;
        }
    }

    emptyDungeon();
    createRooms();
    connectRooms();
    placeStairs();
    placePlayer();
    initializeHardness();

    if (numMonsters == 0) {
        printf("Dungeon:\n");
        printDungeon();
        printf("\nHardness:\n");
        printHardness();
        printf("\nNon-Tunneling Distance Map:\n");
        printNonTunnelingMap();
        printf("\nTunneling Distance Map:\n");
        printTunnelingMap();

        if (save) {
            if (saveFileName) {
                saveDungeon(saveFileName);
            } else {
                printf("Error: No file path specified for saving!\n");
                return 1;
            }
        }
        return 0;
    }

    if (spawnMonsters(numMonsters)) {
        printf("Failed to spawn monsters\n");
        return 1;
    }

    printf("Initial Dungeon:\n");
    printDungeon();

    MinHeap *eventQueue = createMinHeap(numMonsters + 1);
    if (!eventQueue) {
        printf("Error: Failed to create event queue\n");
        for (int i = 0; i < numMonsters; i++) {
            if (monsters[i]) free(monsters[i]);
        }
        free(monsters);
        return 1;
    }

    for (int i = 0; i < numMonsters; i++) {
        if (monsters[i] && monsters[i]->alive) {
            Event event = {1000 / monsters[i]->speed, monsters[i]};
            HeapNode node = {monsters[i]->x, monsters[i]->y, event.time};
            insertHeap(eventQueue, node);
        }
    }

    Event playerEvent = {1000 / 10, NULL};
    HeapNode playerNode = {player_x, player_y, playerEvent.time};
    insertHeap(eventQueue, playerNode);

    int turn = 0;
    while (eventQueue->size > 0) {
        HeapNode nextEventNode = extractMin(eventQueue);
        int curr_x = nextEventNode.x;
        int curr_y = nextEventNode.y;
        Monster *monster = monsterAt[curr_y][curr_x];

        if (monster) {
            if (!monster->alive) {
                continue;
            }
            moveMonster(monster);

            Monster *culprit = NULL;
            if (isGameOver(&culprit)) {
                int personality = culprit->intelligent + 
                                  (culprit->telepathic << 1) + 
                                  (culprit->tunneling << 2) + 
                                  (culprit->erratic << 3);
                char symbol = personality < 10 ? '0' + personality : 'a' + (personality - 10);
                printf("\nTurn %d: Monster '%c' reached '@' at (%d, %d)!\n", 
                       turn, symbol, player_x, player_y);
                printDungeon();
                printf("GAME OVER: Player has been defeated!\n");
                break;
            }

            if (monster->alive) {
                int nextTime = nextEventNode.distance + (1000 / monster->speed);
                HeapNode newEvent = {monster->x, monster->y, nextTime};
                insertHeap(eventQueue, newEvent);
            }
        } else {
            movePlayer();

            Monster *culprit = NULL;
            if (isGameOver(&culprit)) {
                int personality = culprit->intelligent + 
                                  (culprit->telepathic << 1) + 
                                  (culprit->tunneling << 2) + 
                                  (culprit->erratic << 3);
                char symbol = personality < 10 ? '0' + personality : 'a' + (personality - 10);
                printf("\nTurn %d: Monster '%c' reached '@' at (%d, %d)!\n", 
                       turn, symbol, player_x, player_y);
                printDungeon();
                printf("GAME OVER: Player has been defeated!\n");
                break;
            }

            int nextTime = nextEventNode.distance + (1000 / 10);
            HeapNode newEvent = {player_x, player_y, nextTime};
            insertHeap(eventQueue, newEvent);
        }

        printf("\nTurn %d: Player at (%d, %d):\n", turn++, player_x, player_y);
        printDungeon();
        usleep(250000);
    }

    free(eventQueue->nodes);
    free(eventQueue);
    for (int i = 0; i < numMonsters; i++) {
        if (monsters[i]) free(monsters[i]);
    }
    free(monsters);

    if (save) {
        if (saveFileName) {
            saveDungeon(saveFileName);
        }
    }

    return 0;
}