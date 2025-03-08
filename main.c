#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <time.h>
#include <unistd.h>
#include "dungeon_generation.h"
#include "minheap.h"

typedef struct {
    int time;         // Event time based on 1000/speed
    Monster *monster; // Pointer to the monster
} Event;

// Function to check if any monster has reached the player's position
int isGameOver(Monster **culprit) {
    for (int i = 0; i < num_monsters; i++) {
        if (monsters[i] && monsters[i]->alive && 
            monsters[i]->x == player_x && monsters[i]->y == player_y) {
            *culprit = monsters[i]; // Return the monster that caused game over
            return 1; // A living monster is at the player's position
        }
    }
    *culprit = NULL;
    return 0; // No monster has reached the player
}

int main(int argc, char *argv[]) {
    srand(time(NULL));
    int load = 0, save = 0, nummonFlag = 0;
    char *saveFileName = NULL;
    char *loadFileName = NULL;
    int numMonsters = 0;

    // Parse command-line arguments
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
        } else if (strcmp(argv[i], "--nummon") == 0) {
            nummonFlag = 1;
            if (i + 1 < argc) {
                char *numStr = argv[i + 1];
                int valid = 1;
                for (int j = 0; numStr[j] != '\0'; j++) {
                    if (!isdigit(numStr[j])) {
                        valid = 0;
                        break;
                    }
                }
                if (valid && strlen(numStr) > 0) {
                    numMonsters = atoi(numStr);
                    if (numMonsters < 1) {
                        printf("Error: --nummon must be a positive integer\n");
                        return 1;
                    }
                } else {
                    printf("Error: --nummon requires a positive integer\n");
                    return 1;
                }
                i++;
            } else {
                printf("Error: --nummon requires a positive integer\n");
                return 1;
            }
        } else {
            printf("Error: Unrecognized argument '%s'\n", argv[i]);
            return 1;
        }
    }

    if (!nummonFlag) {
        numMonsters = 10; // Default number of monsters
    }

    if (load) {
        if (loadFileName) {
            loadDungeon(loadFileName);
        } else {
            printf("Error: No file path specified for loading!\n");
            return 1;
        }
    } else {
        emptyDungeon();
        createRooms();
        connectRooms();
        placeStairs();
        placePlayer();
        initializeHardness();
    }

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

    MinHeap *eventQueue = createMinHeap(numMonsters);
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

    int turn = 0;
    while (eventQueue->size > 0) {
        HeapNode nextEventNode = extractMin(eventQueue);
        int curr_x = nextEventNode.x;
        int curr_y = nextEventNode.y;
        Monster *monster = monsterAt[curr_y][curr_x];

        if (!monster || !monster->alive) {
            continue; // Skip if monster is dead or missing
        }

        moveMonster(monster);

        // Check for game over after every monster move
        Monster *culprit = NULL;
        if (isGameOver(&culprit)) {
            // Find the monster's symbol by its index in the monsters array
            char symbol = '0'; // Default fallback
            for (int i = 0; i < num_monsters; i++) {
                if (monsters[i] == culprit) {
                    if (i < 10) {
                        symbol = '0' + i; // 0-9
                    } else {
                        symbol = 'a' + (i - 10); // a-f
                    }
                    break;
                }
            }
            printf("\nTurn %d: Monster '%c' reached '@' at (%d, %d)!\n", 
                   turn, symbol, player_x, player_y);
            printDungeon();
            printf("GAME OVER: Player has been defeated!\n");
            break; // Exit the game loop immediately
        }

        if (monster->alive) {
            int nextTime = nextEventNode.distance + (1000 / monster->speed);
            HeapNode newEvent = {monster->x, monster->y, nextTime};
            insertHeap(eventQueue, newEvent);
        }

        printf("\nTurn %d: After monster at (%d, %d) moves:\n", turn++, curr_x, curr_y);
        printDungeon();
        usleep(250000);
    }

    if (save) {
        if (saveFileName) {
            saveDungeon(saveFileName);
        } else {
            printf("Error: No file path specified for saving!\n");
            return 1;
        }
    }

    free(eventQueue->nodes);
    free(eventQueue);

    for (int i = 0; i < num_monsters; i++) {
        if (monsters[i]) free(monsters[i]);
    }
    free(monsters);

    return 0;
}