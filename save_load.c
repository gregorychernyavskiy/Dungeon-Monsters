#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <endian.h>

#include "dungeon_generation.h"

char *dungeon_file;

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
    player.x = (int) pos[0];
    player.y = (int) pos[1];

    for (int i = 0; i < MAX_HEIGHT; i++) {
        for (int j = 0; j < MAX_WIDTH; j++) {
            fread(&dungeon[i][j].hardness, 1, 1, file);
            if (dungeon[i][j].hardness == 0) {
                dungeon[i][j].type = CORRIDOR;
            }
            else {
                dungeon[i][j].type = ROCK;
            }
        }
    }
    for (int i = 1; i < MAX_WIDTH - 1; i++) {
        dungeon[0][i].type = '-';
        dungeon[MAX_HEIGHT - 1][i].type = '-';
    }
    for (int i = 1; i < MAX_HEIGHT - 1; i++) {
        dungeon[i][0].type = '|';
        dungeon[i][MAX_WIDTH - 1].type = '|';
    }
    dungeon[0][0].type = CORNER;
    dungeon[0][MAX_WIDTH - 1].type = '+';
    dungeon[MAX_HEIGHT - 1][0].type = '+';
    dungeon[MAX_HEIGHT - 1][MAX_WIDTH - 1].type = '+';

    uint16_t r = 0;
    fread(&r, 2, 1, file);
    r = be16toh(r);
    roomCount = r;

    Room *rooms = malloc(r * sizeof(Room));
    if (!rooms) {
        fprintf(stderr, "Memory allocation failed for rooms\n");
        exit(EXIT_FAILURE);
    }
    for (int i = 0; i < r; i++) {
        fread(&rooms[i].x, 1, 1, file);
        fread(&rooms[i].y, 1, 1, file);
        fread(&rooms[i].width, 1, 1, file);
        fread(&rooms[i].height, 1, 1, file);
    }
    for (int i = 0; i < r; i++) {
        for (int j = rooms[i].y; j < rooms[i].y + rooms[i].height; j++) {
            for (int k = rooms[i].x; k < rooms[i].x + rooms[i].width; k++) {
                dungeon[j][k].type = FLOOR;
            }
        }
    }

    uint16_t u;
    fread(&u, 2, 1, file);
    u = be16toh(u);
    upStairsCount = u;

    Pos *upStairs = malloc(u * sizeof(Pos));
    if (!upStairs) {
        fprintf(stderr, "Memory allocation failed for upStairs\n");
        exit(EXIT_FAILURE);
    }
    for (int i = 0; i < u; i++) {
        fread(&upStairs[i].x, 1, 1, file);
        fread(&upStairs[i].y, 1, 1, file);

        dungeon[upStairs[i].y][upStairs[i].x].type = STAIR_UP;
    }

    uint16_t d;
    fread(&d, 2, 1, file);
    d = be16toh(d);
    downStairsCount = d;

    Pos *downStairs = malloc(d * sizeof(Pos));
    if (!downStairs) {
        fprintf(stderr, "Memory allocation failed for downStairs\n");
        exit(EXIT_FAILURE);
    }
    for (int i = 0; i < d; i++) {
        fread(&downStairs[i].x, 1, 1, file);
        fread(&downStairs[i].y, 1, 1, file);

        dungeon[downStairs[i].y][downStairs[i].x].type = STAIR_DOWN;
    }

    dungeon[player.y][player.x].type = '@';

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

    uint32_t size = htobe32(sizeof(1712 + roomCount * 4));
    fwrite(&size, 4, 1, file);

    uint8_t pos[2] = {(uint8_t) player.x, (uint8_t) player.y};
    fwrite(pos, 2, 1, file);

    for (int i = 0; i < MAX_HEIGHT; i++) {
        for (int j = 0; j < MAX_WIDTH; j++) {
            fwrite(&dungeon[i][j].hardness, 1, 1, file);
        }
    }

    uint16_t r = htobe16(roomCount);
    fwrite(&r, 2, 1, file);

    for (int i = 0; i < roomCount; i++) {
        uint8_t room[4] = {rooms[i].x, rooms[i].y, rooms[i].width, rooms[i].height};
        fwrite(room, 4, 1, file);
    }

    uint16_t u = htobe16(upStairsCount);
    fwrite(&u, 2, 1, file);

    for (int i = 0; i < upStairsCount; i++) {
        uint8_t upStairsPos[2] = {(uint8_t) upStairs[i].x, (uint8_t) upStairs[i].y};
        fwrite(upStairsPos, 2, 1, file);
    }

    uint16_t d = htobe16(downStairsCount);
    fwrite(&d, 2, 1, file);

    for (int i = 0; i < downStairsCount; i++) {
        uint8_t downStairsPos[2] = {(uint8_t) downStairs[i].x, (uint8_t) downStairs[i].y};
        fwrite(downStairsPos, 2, 1, file);
    }

    printf("Dungeon saved to %s\n", dungeon_file);
    fclose(file);
    free(dungeon_file);
}