#include "dungeon_generation.h"

int main(int argc, char *argv[]) {
    srand(time(NULL));
    int load = 0, save = 0;
    char *saveFileName = NULL; // To store the filename for saving

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--save") == 0) {
            save = 1;
            if (i + 1 < argc) { // Check if a filename follows
                saveFileName = argv[i + 1];
                i++; // Skip the next argument since it's the filename
            } else {
                printf("Error: Missing filename for --save\n");
                return 1;
            }
        } else if (strcmp(argv[i], "--load") == 0) {
            load = 1;
        }
    }

    if (load) {
        loadDungeon();
    } else {
        emptyDungeon();
        createRooms();
        connectRooms();
        placeStairs();
        placePlayer();
        initializeHardness();
    }

    printDungeon();
    printHardness();

    if (save) {
        if (saveFileName) {
            saveDungeon(saveFileName); // Pass the filename to saveDungeon()
        } else {
            printf("Error: No file path specified for saving!\n");
            return 1;
        }
    }

    return 0;
}