#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "dungeon_generation.h"

int main(int argc, char *argv[]) {
    srand(time(NULL));
    int load = 0, save = 0;
    char *saveFileName = "dungeon";
    char *loadFileName = "dungeon";

    // Parse arguments
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
        }
    }

    // Generate or load dungeon
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

    // Print dungeon
    printf("Dungeon:\n");
    printDungeon();

    // Save if requested
    if (save) {
        saveDungeon(saveFileName);
    }

    return 0;
}