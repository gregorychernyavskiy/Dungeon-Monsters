#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <time.h>
#include <unistd.h>
#include "dungeon_generation.h"
#include "minheap.h"

int main(int argc, char *argv[]) {
    srand(time(NULL));

    // Case 1: No arguments - Old functionality
    if (argc == 1) {
        emptyDungeon();
        createRooms();
        connectRooms();
        placeStairs();
        placePlayer();
        initializeHardness();

        printf("Generated Dungeon:\n");
        printDungeon();
        printf("\nHardness Map:\n");
        printHardness();
        printf("\nNon-Tunneling Distance Map:\n");
        printNonTunnelingMap();
        printf("\nTunneling Distance Map:\n");
        printTunnelingMap();

        return 0;
    }

    // Case 2: One argument - Game loop with monsters
    if (argc == 2) {
        int numMonsters;
        char *numStr = argv[1];
        int valid = 1;

        // Validate that the argument is a positive integer
        for (int j = 0; numStr[j] != '\0'; j++) {
            if (!isdigit(numStr[j])) {
                valid = 0;
                break;
            }
        }

        if (!valid || strlen(numStr) == 0) {
            printf("Error: Argument must be a positive integer\n");
            return 1;
        }

        numMonsters = atoi(numStr);
        if (numMonsters < 1 || numMonsters > 15) {
            printf("Error: Number of monsters must be between 1 and 15\n");
            return 1;
        }

        // Initialize dungeon
        emptyDungeon();
        createRooms();
        connectRooms();
        placeStairs();
        placePlayer();
        initializeHardness();

        // Initialize monsterAt array
        for (int y = 0; y < HEIGHT; y++) {
            for (int x = 0; x < WIDTH; x++) {
                monsterAt[y][x] = NULL;
            }
        }

        // Spawn monsters
        if (spawnMonsters(numMonsters)) {
            printf("Failed to spawn monsters\n");
            return 1;
        }

        printf("Initial Dungeon:\n");
        printDungeon();

        // Create event queue
        MinHeap *eventQueue = createMinHeap(numMonsters + 1); // +1 for player
        if (!eventQueue) {
            printf("Error: Failed to create event queue\n");
            for (int i = 0; i < num_monsters; i++) {
                if (monsters[i]) free(monsters[i]);
            }
            free(monsters);
            return 1;
        }

        // Insert initial events for all monsters
        for (int i = 0; i < num_monsters; i++) {
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

                // Check for game over after monster move
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

                // Check for game over after player move
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
        for (int i = 0; i < num_monsters; i++) {
            if (monsters[i]) free(monsters[i]);
        }
        free(monsters);

        return 0;
    }

    // Invalid number of arguments
    printf("Usage: ./dungeon [num_monsters]\n");
    printf("  - No arguments: Generate and print dungeon, hardness, and distance maps\n");
    printf("  - One argument (1-15): Run game with specified number of monsters\n");
    return 1;
}