#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "dungeon_generation.h"
#include "minheap.h"

int main(int argc, char *argv[]) {
    srand(time(NULL));
    int load = 0, save = 0, numMonsters = 0;
    char *saveFileName = "dungeon"; // Default filename
    char *loadFileName = "dungeon"; // Default filename

    // Parse command-line arguments
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--save") == 0) {
            save = 1;
            if (i + 1 < argc && argv[i + 1][0] != '-') {
                saveFileName = argv[++i];
            }
        } else if (strcmp(argv[i], "--load") == 0) {
            load = 1;
            if (i + 1 < argc && argv[i + 1][0] != '-') {
                loadFileName = argv[++i];
            }
        } else if (strcmp(argv[i], "--nummon") == 0) {
            if (i + 1 < argc && isdigit(argv[i + 1][0])) {
                numMonsters = atoi(argv[++i]);
                if (numMonsters < 1 || numMonsters > 15) {
                    printf("Error: Number of monsters must be between 1 and 15\n");
                    printf("Usage: ./dungeon [--save [filename]] [--load [filename]] [--nummon <1-15>]\n");
                    return 1;
                }
            } else {
                printf("Error: --nummon requires a number between 1 and 15\n");
                printf("Usage: ./dungeon [--save [filename]] [--load [filename]] [--nummon <1-15>]\n");
                return 1;
            }
        } else {
            printf("Error: Unrecognized argument '%s'\n", argv[i]);
            printf("Usage: ./dungeon [--save [filename]] [--load [filename]] [--nummon <1-15>]\n");
            return 1;
        }
    }

    // Initialize monsterAt array (needed if monsters are used)
    for (int y = 0; y < HEIGHT; y++) {
        for (int x = 0; x < WIDTH; x++) {
            monsterAt[y][x] = NULL;
        }
    }

    // Load or generate dungeon
    if (load) {
        loadDungeon(loadFileName);
    } else {
        emptyDungeon();
        createRooms();
        connectRooms();
        placeStairs();
        placePlayer();
        initializeHardness();
    }

    // If no monsters, just print maps
    if (numMonsters == 0) {
        printf("Dungeon:\n");
        printDungeon();
        //printf("\nHardness:\n");
        //printHardness(); // Commented out as in your old main
        printf("\nNon-Tunneling Distance Map:\n");
        printNonTunnelingMap();
        printf("\nTunneling Distance Map:\n");
        printTunnelingMap();

        if (save) {
            saveDungeon(saveFileName);
        }
        return 0;
    }

    // Monster game mode
    if (spawnMonsters(numMonsters)) {
        printf("Failed to spawn monsters\n");
        return 1;
    }

    printf("Initial Dungeon:\n");
    printDungeon();

    MinHeap *eventQueue = createMinHeap(numMonsters + 1); // +1 for player
    if (!eventQueue) {
        printf("Error: Failed to create event queue\n");
        for (int i = 0; i < numMonsters; i++) {
            if (monsters[i]) free(monsters[i]);
        }
        free(monsters);
        return 1;
    }

    // Insert initial events for monsters
    for (int i = 0; i < numMonsters; i++) {
        if (monsters[i] && monsters[i]->alive) {
            Event event = {0, monsters[i]};
            HeapNode node = {monsters[i]->x, monsters[i]->y, event.time};
            insertHeap(eventQueue, node);
        }
    }

    // Insert initial event for player (speed = 10)
    Event playerEvent = {0, NULL}; // NULL indicates player
    HeapNode playerNode = {player_x, player_y, playerEvent.time};
    insertHeap(eventQueue, playerNode);

    int turn = 0;
    while (eventQueue->size > 0) {
        HeapNode nextEventNode = extractMin(eventQueue);
        int curr_x = nextEventNode.x;
        int curr_y = nextEventNode.y;
        Monster *monster = monsterAt[curr_y][curr_x];

        if (monster) { // Monster move
            if (!monster->alive) continue;
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
        } else { // Player move
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

            int nextTime = nextEventNode.distance + (1000 / 10); // Player speed = 10
            HeapNode newEvent = {player_x, player_y, nextTime};
            insertHeap(eventQueue, newEvent);
        }

        printf("\nTurn %d: Player at (%d, %d):\n", turn++, player_x, player_y);
        printDungeon();
        usleep(250000);
    }

    // Cleanup
    free(eventQueue->nodes);
    free(eventQueue);
    for (int i = 0; i < numMonsters; i++) {
        if (monsters[i]) free(monsters[i]);
    }
    free(monsters);

    if (save) {
        saveDungeon(saveFileName);
    }

    return 0;
}