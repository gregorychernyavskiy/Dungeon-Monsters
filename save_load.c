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

    dungeonFile = malloc(fileLength * sizeof(char));
    if (!dungeonFile) {
        perror("Memory allocation failed...");
        exit(EXIT_FAILURE);
    }

    strcpy(dungeonFile, home);
    strcat(dungeonFile, "/.rlg327/");
    strcat(dungeonFile, nameOfFile);
}

void loadDungeon(char *nameOfFile) {

}

void saveDungeon(char *nameOfFile) {
    setupDungeonFile(nameOfFile);
    FILE *file = fopen(dungeonFile, "w");

    if (!file) {
        perror("Error! Cannot open the file...");
        exit(EXIT_FAILURE);
    }

    fwrite("RLG327-S2025", 1, 12, file);
    
    uint32_t versionMaker = htobe32(0);
    fwrite(&versionMaker, 4, 1, file);

    uint32_t size = htobe32(sizeof(1712 + num_rooms * 4));
    fwrite(&size, 4, 1, file);

    fclose(file);
}
