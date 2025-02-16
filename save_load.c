#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <endian.h>

#include "dungeon_generation.h"

char *dungeon_file;
int upStairsCount, downStairsCount;
struct Room *upStairs, *downStairs;

void setupDungeonFile(char *filename) {
    char *home = getenv("HOME");
    int dungeon_file_length = strlen(home) + strlen("/.rlg327/") + strlen(filename) + 1;

    dungeon_file = malloc(dungeon_file_length * sizeof(*dungeon_file));
    if (!dungeon_file) {
        perror("Memory allocation failed for dungeon_file");
        exit(EXIT_FAILURE);
    }

    strcpy(dungeon_file, home);
    strcat(dungeon_file, "/.rlg327/");
    strcat(dungeon_file, filename);
}

void loadDungeon(char *filename) {
    setupDungeonFile(filename);
    FILE *file = fopen(dungeon_file, "r");
    if (!file) {
        perror("Error opening file");
        exit(EXIT_FAILURE);
    }

    char marker[12];
    fread(marker, 1, 12, file);

    uint32_t version;
    fread(&version, 4, 1, file);
    version = be32toh(version);

    uint32_t size;
    fread(&size, 4, 1, file);
    size = be32toh(size);

    uint8_t pos[2];
    fread(pos, 2, 1, file);
    player_x = (int) pos[0];
    player_y = (int) pos[1];

    for (int i = 0; i < HEIGHT; i++) {
        for (int j = 0; j < WIDTH; j++) {
            fread(&hardness[i][j], 1, 1, file);
            dungeon[i][j] = (hardness[i][j] == 0) ? '#' : ' ';
        }
    }
    
    uint16_t r;
    fread(&r, 2, 1, file);
    r = be16toh(r);
    num_rooms = r;

    fread(rooms, sizeof(struct Room), r, file);
    
    uint16_t u;
    fread(&u, 2, 1, file);
    u = be16toh(u);
    upStairsCount = u;
    upStairs = malloc(u * sizeof(struct Room));
    fread(upStairs, sizeof(struct Room), u, file);
    
    uint16_t d;
    fread(&d, 2, 1, file);
    d = be16toh(d);
    downStairsCount = d;
    downStairs = malloc(d * sizeof(struct Room));
    fread(downStairs, sizeof(struct Room), d, file);
    
    dungeon[player_y][player_x] = '@';

    printf("Dungeon loaded from %s\n", dungeon_file);
    fclose(file);
    free(dungeon_file);
}

void saveDungeon(char *filename) {
    setupDungeonFile(filename);
    FILE *file = fopen(dungeon_file, "w");
    if (!file) {
        perror("Error opening file");
        exit(EXIT_FAILURE);
    }

    fwrite("RLG327-S2025", 1, 12, file);

    uint32_t version = htobe32(0);
    fwrite(&version, 4, 1, file);

    uint32_t size = htobe32(sizeof(1712 + num_rooms * 4));
    fwrite(&size, 4, 1, file);

    uint8_t pos[2] = {(uint8_t) player_x, (uint8_t) player_y};
    fwrite(pos, 2, 1, file);

    fwrite(hardness, sizeof(hardness), 1, file);
    
    uint16_t r = htobe16(num_rooms);
    fwrite(&r, 2, 1, file);
    fwrite(rooms, sizeof(struct Room), num_rooms, file);

    uint16_t u = htobe16(upStairsCount);
    fwrite(&u, 2, 1, file);
    fwrite(upStairs, sizeof(struct Room), upStairsCount, file);

    uint16_t d = htobe16(downStairsCount);
    fwrite(&d, 2, 1, file);
    fwrite(downStairs, sizeof(struct Room), downStairsCount, file);

    printf("Dungeon saved to %s\n", dungeon_file);
    fclose(file);
    free(dungeon_file);
}