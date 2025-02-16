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

void loadDungeon() {

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
        fwrite(&room, 4, 1, file);
    }

    //Offset 1704 + r × 4


    fclose(file);
}
