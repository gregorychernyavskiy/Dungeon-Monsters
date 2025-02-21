#include "dungeon_generation.h"
#include "pathfinding.h"

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

    // Standard dungeon view
    printf("Standard Dungeon View:\n");
    printDungeon();

    // Non-tunneling distance map
    uint32_t non_tunneling_distances[HEIGHT][WIDTH];
    dijkstra_non_tunneling(non_tunneling_distances, player_x, player_y);
    printf("\nNon-Tunneling Monster Distance Map:\n");
    print_distance_map(non_tunneling_distances);

    // Tunneling distance map
    uint32_t tunneling_distances[HEIGHT][WIDTH];
    dijkstra_tunneling(tunneling_distances, player_x, player_y);
    printf("\nTunneling Monster Distance Map:\n");
    print_distance_map(tunneling_distances);

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