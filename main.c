#include "dungeon_generation.h"

int main(int argc, char *argv[]) {
    srand(time(NULL));
    int load = 0, save = 0, nummon = 10; // Default 10 monsters
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
        } else if (strcmp(argv[i], "--nummon") == 0) {
            if (i + 1 < argc) {
                nummon = atoi(argv[i + 1]);
                if (nummon < 1) {
                    printf("Error: Number of monsters must be at least 1\n");
                    return 1;
                }
                i++;
            } else {
                printf("Error: Missing number for --nummon\n");
                return 1;
            }
        } else {
            printf("Error: Unrecognized argument '%s'\n", argv[i]);
            return 1;
        }
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

    // Spawn monsters and run the game
    spawnMonsters(nummon);
    printf("Initial Dungeon:\n");
    printDungeon();
    printf("\nNon-Tunneling Distance Map:\n");
    printNonTunnelingMap();
    printf("\nTunneling Distance Map:\n");
    printTunnelingMap();

    runGame();

    if (save && !load) {
        if (saveFileName) {
            saveDungeon(saveFileName);
        } else {
            printf("Error: No file path specified for saving!\n");
            return 1;
        }
    }

    return 0;
}