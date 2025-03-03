#include "dungeon_generation.h"

int main(int argc, char *argv[]) {
    srand(time(NULL));
    int load = 0, save = 0;
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

    // Calculate distance maps before printing
    dijkstraNonTunneling(distance_non_tunnel);
    dijkstraTunneling(distance_tunnel);

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