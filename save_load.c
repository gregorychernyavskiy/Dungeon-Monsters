#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <endian.h>

#include "dungeon_generation.h"

char *dungeonFile;

void setupDungeonFile(char *nameOfFile) {
    char *home = getenv("HOME");
    int fileLength = strlen(home) + strlen("/.rlg327/") + strlen(nameOfFile) + 1;

    dungeonFile = malloc(fileLength);
    if (!dungeonFile) {
        perror("Memory allocation failed...");
        exit(EXIT_FAILURE);
    }

    strcpy(dungeonFile, home);
    strcat(dungeonFile, "/.rlg327/");
    strcat(dungeonFile, nameOfFile);
}

void saveDungeon(char *nameOfFile) {
    setupDungeonFile(nameOfFile);
    FILE *file = fopen(dungeonFile, "w");

    if (!file) {
        perror("Error! Cannot open the file...");
        exit(EXIT_FAILURE);
    }
    //Offset 0
    fwrite("RLG327-S2025", 1, 12, file);
    
    //Offset 12
    uint32_t versionMaker = htobe32(0);
    fwrite(&versionMaker, 4, 1, file);

    //Offset 16
    uint32_t sizeOfTheFile = htobe32(1712 + num_rooms * 4);
    fwrite(&sizeOfTheFile, 4, 1, file);

    //Offset 20
    uint8_t position[2] = {(uint8_t) player_x, (uint8_t) player_y};
    fwrite(&position, 2, 1, file);

    //Offset 22
    for (int y = 0; y < HEIGHT; y++) {
        for (int x = 0; x < WIDTH; x++) {
            fwrite(&hardness[y][x], 1, 1, file);
        }
    }

    //Offset 1702
    uint16_t r = htobe16(num_rooms);
    fwrite(&r, 2, 1, file);

    //Offset 1704
    for (int i = 0; i < num_rooms; i++) {
        uint8_t room[4] = {rooms[i].x, rooms[i].y, rooms[i].width, rooms[i].height};
        fwrite(room, 4, 1, file);
    }

    //Offset 1704 + r × 4
    uint16_t upstairs = htobe16(upStairsCount);
    fwrite(&upstairs, 2, 1, file);
    //Write upstairs positions
    for (int i = 0; i < upStairsCount; i++) {
        uint8_t upStairsNum[2] = {(uint8_t) upStairs[i].x, (uint8_t) upStairs[i].y};
        fwrite(upStairsNum, 2, 1, file);
    }

    //Offset 1704 + r × 4 + u × 2
    uint16_t downstairs = htobe16(downStairsCount);
    fwrite(&downstairs, 2, 1, file);
    //Write upstairs positions
    for (int i = 0; i < downStairsCount; i++) {
        uint8_t downPos[2] = {(uint8_t) downStairs[i].x, (uint8_t) downStairs[i].y};
        fwrite(downPos, 2, 1, file);
    }

    printf("Dungeon saved to %s\n", dungeonFile);
    fclose(file);
    free(dungeonFile);
}



void loadDungeon(char *nameOfFile) {
    setupDungeonFile(nameOfFile);
    FILE *file = fopen(dungeonFile, "r");

    if (!file) {
        perror("Error! Cannot open the file...");
        exit(EXIT_FAILURE);
    }

    //Offset 0
    char marker[12];
    fread(marker, 1, 12, file);

    //Offset 12
    uint32_t versionMaker;
    fread(&versionMaker, 4, 1, file);
    versionMaker = be32toh(versionMaker);

    //Offset 16
    uint32_t sizeOfTheFile;
    fread(&sizeOfTheFile, 4, 1, file);
    sizeOfTheFile = be32toh(sizeOfTheFile);

    //Offset 20
    uint8_t position[2];
    fread(&position, 2, 1, file);
    player_x = position[0];
    player_y = position[1];

    //Offset 22
    for (int y = 0; y < HEIGHT; y++) {
        for (int x = 0; x < WIDTH; x++) {
            fread(&hardness[y][x], 1, 1, file);
            //Restore dungeon terrain based on hardness
            if (hardness[y][x] == 0) {
                dungeon[y][x] = '#';
            } else if (hardness[y][x] == 255) {
                dungeon[y][x] = '+';
            } else {
                dungeon[y][x] = ' ';
            }
        }
    }

    //Offset 1702
    uint16_t r;
    fread(&r, 2, 1, file);
    num_rooms = be16toh(r);

    //Offset 1704
    for (int i = 0; i < num_rooms; i++) {
        uint8_t room[4];
        fread(room, 4, 1, file);
        rooms[i].x = room[0];
        rooms[i].y = room[1];
        rooms[i].width = room[2];
        rooms[i].height = room[3];

        //Restore rooms
        for (int y = rooms[i].y; y < rooms[i].y + rooms[i].height; y++) {
            for (int x = rooms[i].x; x < rooms[i].x + rooms[i].width; x++) {
                dungeon[y][x] = '.';
            }
        }
    }

    //Offset 1704 + r × 4
    uint16_t upstairs;
    fread(&upstairs, 2, 1, file);
    upStairsCount = be16toh(upstairs);
    //Read upstairs positions
    for (int i = 0; i < upStairsCount; i++) {
        uint8_t upPos[2];
        fread(upPos, 2, 1, file);
        upStairs[i].x = upPos[0];
        upStairs[i].y = upPos[1];
        dungeon[upStairs[i].y][upStairs[i].x] = '<';
    }

    //Offset 1704 + r × 4 + u × 2
    uint16_t downstairs;
    fread(&downstairs, 2, 1, file);
    downStairsCount = be16toh(downstairs);
    //Write upstairs positions
    for (int i = 0; i < downStairsCount; i++) {
        uint8_t downPos[2];
        fread(downPos, 2, 1, file);
        downStairs[i].x = downPos[0];
        downStairs[i].y = downPos[1];
        dungeon[downStairs[i].y][downStairs[i].x] = '>';
    }

    //Set player position
    dungeon[player_y][player_x] = '@';

    printf("Dungeon loaded from %s\n", dungeonFile);
    fclose(file);
    free(dungeonFile);
}