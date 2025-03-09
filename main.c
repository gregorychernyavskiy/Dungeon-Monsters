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
    int load = 0, save = 0;
    char *saveFileName = NULL;
    char *loadFileName = NULL;
    int numMonsters = 0;

    // Parse command-line arguments
    if (argc > 1) {
        if (strcmp(argv[1], "--save") == 0) {
            save = 1;
            if (argc == 3) {
                saveFileName = argv[2];
            } else {
                printf("Error: Missing filename for --save\n");
                printf("Usage: ./dungeon [--save <filename>] [--load <filename>] [<num_monsters> (1-15)]\n");
                return 1;
            }
        } else if (strcmp(argv[1], "--load") == 0) {
            load = 1;
            if (argc == 3) {
                loadFileName = argv[2];
            } else {
                printf("Error: Missing filename for --load\n");
                printf("Usage: ./dungeon [--save <filename>] [--load <filename>] [<num_monsters> (1-15)]\n");
                return 1;
            }
        } else if (argc == 2) {
            // Check if it's a number of monsters
            char *numStr = argv[1];
            int valid = 1;
            for (int j = 0; numStr[j] != '\0'; j++) {
                if (!isdigit(numStr[j])) {
                    valid = 0;
                    break;
                }
            }
            if (valid && strlen(numStr) > 0) {
                numMonsters = atoi(numStr);
                if (numMonsters < 1 || numMonsters > 15) {
                    printf("Error: Number of monsters must be between 1 and 15\n");
                    printf("Usage: ./dungeon [--save <filename>] [--load <filename>] [<num_monsters> (1-15)]\n");
                    return 1;
                }
            } else {
                printf("Error: Unrecognized argument '%s'\n", numStr);
                printf("Usage: ./dungeon [--save <filename>] [--load <filename>] [<num_monsters> (1-15)]\n");
                return 1;
            }
        } else {
            printf("Error: Invalid arguments\n");
            printf("Usage: ./dungeon [--save <filename>] [--load <filename>] [<num_monsters> (1-15)]\n");
            return 1;
        }
    }

    // If no arguments or only --save/--load
    if (argc == 1 || save || load) {
        if (load) {
            loadDungeon(loadFileName);
        } else { // Default or --save
            emptyDungeon();
            createRooms();
            connectRooms();
            placeStairs();
            placePlayer();
            initializeHardness();
        }

        // Print dungeon and maps
        printf("Dungeon:\n");
        printDungeon();
        printf("\nHardness Map:\n");
        printHardness();
        printf("\nNon-Tunneling Distance Map:\n");
        printNonTunnelingMap();
        printf("\nTunneling Distance Map:\n");
        printTunnelingMap();

        if (save) {
            saveDungeon(saveFileName);
        }
        return 0;
    }

    // Case: Run game with monsters
    emptyDungeon();
    createRooms();
    connectRooms();
    placeStairs();
    placePlayer();
    initializeHardness();

    for (int y = 0; y < HEIGHT; y++) {
        for (int x = 0; x < WIDTH; x++) {
            monsterAt[y][x] = NULL;
        }
    }

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

    // Insert initial events for all monsters
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

    return 0;
}