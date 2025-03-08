#include "dungeon_generation.h"
#include "minheap.h"
#include <unistd.h>
#include <ctype.h>

typedef struct {
    int time;         // Event time based on 1000/speed
    Monster *monster; // Pointer to the monster
} Event;

int main(int argc, char *argv[]) {
    srand(time(NULL));
    int load = 0, save = 0, nummonFlag = 0;
    char *saveFileName = NULL;
    char *loadFileName = NULL;
    int numMonsters = 0; // Will be set by --nummon or default to 10

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

    // Set default number of monsters if --nummon not provided
    if (!nummonFlag) {
        numMonsters = 10; // Hardcoded default per assignment
    }

    // Initialize dungeon
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

    // Clear monsterAt array
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

    // Print initial dungeon
    printf("Initial Dungeon:\n");
    printDungeon();

    // Set up priority queue for monster movement
    MinHeap *eventQueue = createMinHeap(numMonsters);
    if (!eventQueue) {
        printf("Error: Failed to create event queue\n");
        for (int i = 0; i < num_monsters; i++) {
            if (monsters[i]) free(monsters[i]);
        }
        free(monsters);
        return 1;
    }

    // Add initial events for all monsters
    for (int i = 0; i < num_monsters; i++) {
        if (monsters[i] && monsters[i]->alive) {
            Event event = {0, monsters[i]}; // Initial time is 0
            HeapNode node = {monsters[i]->x, monsters[i]->y, event.time};
            insertHeap(eventQueue, node);
        }
    }

    // Game loop
    int turn = 0;
    while (eventQueue->size > 0) {
        HeapNode nextEventNode = extractMin(eventQueue);
        int curr_x = nextEventNode.x;
        int curr_y = nextEventNode.y;
        Monster *monster = monsterAt[curr_y][curr_x];

        if (!monster || !monster->alive) {
            continue; // Skip if monster is dead or missing
        }

        // Move monster
        moveMonster(monster);

        // Check if monster reached player
        if (monster->x == player_x && monster->y == player_y) {
            printf("\nTurn %d: Monster reached '@'!\n", turn);
            printDungeon();
            printf("GAME OVER\n");
            break; // Stop the game immediately
        }

        // Schedule next move if monster is still alive
        if (monster->alive) {
            int nextTime = nextEventNode.distance + (1000 / monster->speed);
            HeapNode newEvent = {monster->x, monster->y, nextTime};
            insertHeap(eventQueue, newEvent);
        }

        // Print dungeon after each monster move
        printf("\nTurn %d: After monster at (%d, %d) moves:\n", turn++, curr_x, curr_y);
        printDungeon();
        usleep(250000); // Pause 250ms as per assignment
    }

    // Cleanup
    if (save) {
        if (saveFileName) {
            saveDungeon(saveFileName);
        } else {
            printf("Error: No file path specified for saving!\n");
            return 1;
        }
    }

    // Free event queue
    free(eventQueue->nodes);
    free(eventQueue);

    // Free monsters
    for (int i = 0; i < num_monsters; i++) {
        if (monsters[i]) free(monsters[i]);
    }
    free(monsters);

    return 0;
}