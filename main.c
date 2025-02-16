#include "dungeon_generation.h"

int main(int argc, char *argv[]) {
    srand(time(NULL));
    int load = 0, save = 0;

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--save") == 0) {
            save = 1;
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
        char filename[256];
        strncpy(filename, argv[argc - 1], sizeof(filename) - 1);
        filename[sizeof(filename) - 1] = '\0';
        saveDungeon();
    }

    return 0;
}