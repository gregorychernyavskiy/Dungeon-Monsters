#include "dungeon_generation.h"

int main(int argc, char *argv[]) {
    srand(time(NULL));
    int load = 0, save = 0;
    char *saveFileName = NULL;
    char *loadFileName = NULL;

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--save") == 0) {
            save = 1;
            if (i + 1 < argc && argv[i+1][0] != '-') {
                saveFileName = argv[i + 1];
                i++;
            } else {
                saveFileName = "dungeon.rlg327";
            }
        } else if (strcmp(argv[i], "--load") == 0) {
            load = 1;
            if (i + 1 < argc && argv[i+1][0] != '-') {
                loadFileName = argv[i + 1];
                i++;
            } else {
                loadFileName = "dungeon.rlg327";
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

    // Print original dungeon
    printf("\nOriginal Dungeon:\n");
    printDungeon();
    
    // Calculate and print the distance maps
    calculate_non_tunneling_distances();
    calculate_tunneling_distances();
    
    print_non_tunneling_distance_map();
    print_tunneling_distance_map();

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