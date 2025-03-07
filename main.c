#include "dungeon_generation.h"

Player pc;
Monster *monsters = NULL; // Define here
int num_monsters = DEFAULT_MONSTERS;

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
        } else if (strcmp(argv[i], "--nummon") == 0) {
            if (i + 1 < argc) {
                num_monsters = atoi(argv[i + 1]);
                if (num_monsters < 1) num_monsters = DEFAULT_MONSTERS;
                i++;
            } else {
                printf("Error: Missing number for --nummon\n");
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

    // Initialize PC
    pc.x = player_x;
    pc.y = player_y;
    pc.speed = PC_SPEED;
    pc.alive = 1;

    // Spawn monsters
    spawnMonsters(num_monsters);

    // Run the game
    runGame();

    // Cleanup
    freeMonsters();
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