#include "dungeon_generation.h"
#include <cstring>
#include <vector>

void setupDungeonFile(char* nameOfFile) {
    char* home = getenv("HOME");
    int fileLength = strlen(home) + strlen("/.rlg327/") + strlen(nameOfFile) + 1;

    dungeonFile = new char[fileLength];
    if (!dungeonFile) {
        perror("Memory allocation failed...");
        exit(EXIT_FAILURE);
    }

    strcpy(dungeonFile, home);
    strcat(dungeonFile, "/.rlg327/");
    strcat(dungeonFile, nameOfFile);
}

void saveDungeon(char* nameOfFile) {
    setupDungeonFile(nameOfFile);
    FILE* file = fopen(dungeonFile, "wb");

    if (!file) {
        perror("Error! Cannot open the file...");
        delete[] dungeonFile;
        exit(EXIT_FAILURE);
    }

    // Offset 0: File marker
    fwrite("RLG327-S2025", 1, 12, file);

    // Offset 12: Version
    uint32_t version = htobe32(1); // Updated version for equipment/inventory
    fwrite(&version, sizeof(version), 1, file);

    // Calculate file size
    uint32_t sizeOfTheFile = 12 + 4 + 4 + 2 + (HEIGHT * WIDTH) + 2 + (num_rooms * 4) +
                             2 + (upStairsCount * 2) + 2 + (downStairsCount * 2) +
                             4 + 12 * 4 + 10 * 4 + 4 + num_monsters * (4 + 4 + 4 + 4 + 1);
    fwrite(&sizeOfTheFile, sizeof(sizeOfTheFile), 1, file);

    // Offset 20: Player position
    uint8_t position[2] = {(uint8_t)player->x, (uint8_t)player->y};
    fwrite(position, sizeof(position), 1, file);

    // Offset 22: Hardness map
    for (int y = 0; y < HEIGHT; y++) {
        for (int x = 0; x < WIDTH; x++) {
            fwrite(&hardness[y][x], sizeof(hardness[y][x]), 1, file);
        }
    }

    // Offset 1702: Number of rooms
    uint16_t r = htobe16(num_rooms);
    fwrite(&r, sizeof(r), 1, file);

    // Offset 1704: Room data
    for (int i = 0; i < num_rooms; i++) {
        uint8_t room[4] = {(uint8_t)rooms[i].x, (uint8_t)rooms[i].y, (uint8_t)rooms[i].width, (uint8_t)rooms[i].height};
        fwrite(room, sizeof(room), 1, file);
    }

    // Offset 1704 + r * 4: Upstairs count and data
    uint16_t upstairs = htobe16(upStairsCount);
    fwrite(&upstairs, sizeof	upstairs, 1, file);
    for (int i = 0; i < upStairsCount; i++) {
        uint8_t upStairsNum[2] = {(uint8_t)upStairs[i].x, (uint8_t)upStairs[i].y};
        fwrite(upStairsNum, sizeof(upStairsNum), 1, file);
    }

    // Offset 1704 + r * 4 + u * 2: Downstairs count and data
    uint16_t downstairs = htobe16(downStairsCount);
    fwrite(&downstairs, sizeof(downstairs), 1, file);
    for (int i = 0; i < downStairsCount; i++) {
        uint8_t downPos[2] = {(uint8_t)downStairs[i].x, (uint8_t)downStairs[i].y};
        fwrite(downPos, sizeof(downPos), 1, file);
    }

    // Save PC stats
    uint32_t pc_hitpoints = htobe32(player->hitpoints);
    fwrite(&pc_hitpoints, sizeof(pc_hitpoints), 1, file);
    uint32_t pc_damage_base = htobe32(player->damage.base);
    fwrite(&pc_damage_base, sizeof(pc_damage_base), 1, file);
    uint32_t pc_damage_dice = htobe32(player->damage.dice);
    fwrite(&pc_damage_dice, sizeof(pc_damage_dice), 1, file);
    uint32_t pc_damage_sides = htobe32(player->damage.sides);
    fwrite(&pc_damage_sides, sizeof(pc_damage_sides), 1, file);

    // Save equipment (12 slots, index into objectDescs or -1 for empty)
    for (int i = 0; i < 12; i++) {
        int32_t index = -1;
        if (player->equipment[i]) {
            for (size_t j = 0; j < objectDescs.size(); j++) {
                if (objectDescs[j].name == player->equipment[i]->name) {
                    index = j;
                    break;
                }
            }
        }
        index = htobe32(index);
        fwrite(&index, sizeof(index), 1, file);
    }

    // Save inventory (10 slots, index into objectDescs or -1 for empty)
    for (int i = 0; i < 10; i++) {
        int32_t index = -1;
        if (player->carry[i]) {
            for (size_t j = 0; j < objectDescs.size(); j++) {
                if (objectDescs[j].name == player->carry[i]->name) {
                    index = j;
                    break;
                }
            }
        }
        index = htobe32(index);
        fwrite(&index, sizeof(index), 1, file);
    }

    // Save monsters
    uint32_t monster_count = htobe32(num_monsters);
    fwrite(&monster_count, sizeof(monster_count), 1, file);
    for (int i = 0; i < num_monsters; i++) {
        int32_t index = -1;
        for (size_t j = 0; j < monsterDescs.size(); j++) {
            if (monsterDescs[j].name == monsters[i]->name) {
                index = j;
                break;
            }
        }
        index = htobe32(index);
        fwrite(&index, sizeof(index), 1, file);
        uint32_t x = htobe32(monsters[i]->x);
        fwrite(&x, sizeof(x), 1, file);
        uint32_t y = htobe32(monsters[i]->y);
        fwrite(&y, sizeof(y), 1, file);
        uint32_t hitpoints = htobe32(monsters[i]->hitpoints);
        fwrite(&hitpoints, sizeof(hitpoints), 1, file);
        uint8_t alive = monsters[i]->alive;
        fwrite(&alive, sizeof(alive), 1, file);
    }

    printf("Dungeon saved to %s\n", dungeonFile);
    fclose(file);
    delete[] dungeonFile;
    dungeonFile = nullptr;
}

void loadDungeon(char* nameOfFile) {
    setupDungeonFile(nameOfFile);
    FILE* file = fopen(dungeonFile, "rb");

    if (!file) {
        perror("Error! Cannot open the file...");
        delete[] dungeonFile;
        exit(EXIT_FAILURE);
    }

    // Offset 0: Verify file marker
    char marker[12];
    fread(marker, 1, 12, file);
    if (strncmp(marker, "RLG327-S2025", 12) != 0) {
        fprintf(stderr, "Error: Invalid file marker in %s\n", dungeonFile);
        fclose(file);
        delete[] dungeonFile;
        exit(EXIT_FAILURE);
    }

    // Offset 12: Version
    uint32_t version;
    fread(&version, sizeof(version), 1, file);
    version = be32toh(version);
    if (version != 1) {
        fprintf(stderr, "Error: Unsupported file version %u in %s\n", version, dungeonFile);
        fclose(file);
        delete[] dungeonFile;
        exit(EXIT_FAILURE);
    }

    // Offset 16: File size
    uint32_t sizeOfTheFile;
    fread(&sizeOfTheFile, sizeof(sizeOfTheFile), 1, file);
    sizeOfTheFile = be32toh(sizeOfTheFile);

    // Offset 20: Player position
    uint8_t position[2];
    fread(position, sizeof(position), 1, file);
    if (player) delete player;
    player = new PC(position[0], position[1]);

    // Offset 22: Hardness map
    for (int y = 0; y < HEIGHT; y++) {
        for (int x = 0; x < WIDTH; x++) {
            fread(&hardness[y][x], sizeof(hardness[y][x]), 1, file);
            if (hardness[y][x] == 0) {
                dungeon[y][x] = '#';
                terrain[y][x] = '#';
            } else if (hardness[y][x] == 255) {
                dungeon[y][x] = '+';
                terrain[y][x] = '+';
            } else {
                dungeon[y][x] = ' ';
                terrain[y][x] = ' ';
            }
        }
    }

    // Offset 1702: Number of rooms
    uint16_t r;
    fread(&r, sizeof(r), 1, file);
    num_rooms = be16toh(r);

    // Offset 1704: Room data
    for (int i = 0; i < num_rooms; i++) {
        uint8_t room[4];
        fread(room, sizeof(room), 1, file);
        rooms[i].x = room[0];
        rooms[i].y = room[1];
        rooms[i].width = room[2];
        rooms[i].height = room[3];
        for (int y = rooms[i].y; y < rooms[i].y + rooms[i].height; y++) {
            for (int x = rooms[i].x; x < rooms[i].x + rooms[i].width; x++) {
                dungeon[y][x] = '.';
                terrain[y][x] = '.';
            }
        }
    }

    // Offset 1704 + r * 4: Upstairs
    uint16_t upstairs;
    fread(&upstairs, sizeof(upstairs), 1, file);
    upStairsCount = be16toh(upstairs);
    for (int i = 0; i < upStairsCount; i++) {
        uint8_t upPos[2];
        fread(upPos, sizeof(upPos), 1, file);
        upStairs[i].x = upPos[0];
        upStairs[i].y = upPos[1];
        dungeon[upStairs[i].y][upStairs[i].x] = '<';
        terrain[upStairs[i].y][upStairs[i].x] = '<';
    }

    // Offset 1704 + r * 4 + u * 2: Downstairs
    uint16_t downstairs;
    fread(&downstairs, sizeof(downstairs), 1, file);
    downStairsCount = be16toh(downstairs);
    for (int i = 0; i < downStairsCount; i++) {
        uint8_t downPos[2];
        fread(downPos, sizeof(downPos), 1, file);
        downStairs[i].x = downPos[0];
        downStairs[i].y = downPos[1];
        dungeon[downStairs[i].y][downStairs[i].x] = '>';
        terrain[downStairs[i].y][downStairs[i].x] = '>';
    }

    // Load PC stats
    uint32_t pc_hitpoints, pc_damage_base, pc_damage_dice, pc_damage_sides;
    fread(&pc_hitpoints, sizeof(pc_hitpoints), 1, file);
    fread(&pc_damage_base, sizeof(pc_damage_base), 1, file);
    fread(&pc_damage_dice, sizeof(pc_damage_dice), 1, file);
    fread(&pc_damage_sides, sizeof(pc_damage_sides), 1, file);
    player->hitpoints = be32toh(pc_hitpoints);
    player->damage.base = be32toh(pc_damage_base);
    player->damage.dice = be32toh(pc_damage_dice);
    player->damage.sides = be32toh(pc_damage_sides);

    // Load equipment
    for (int i = 0; i < 12; i++) {
        int32_t index;
        fread(&index, sizeof(index), 1, file);
        index = be32toh(index);
        if (index >= 0 && index < (int32_t)objectDescs.size()) {
            player->equipment[i] = objectDescs[index].createObject(player->x, player->y);
        }
    }

    // Load inventory
    for (int i = 0; i < 10; i++) {
        int32_t index;
        fread(&index, sizeof(index), 1, file);
        index = be32toh(index);
        if (index >= 0 && index < (int32_t)objectDescs.size()) {
            Object* obj = objectDescs[index].createObject(player->x, player->y);
            player->carry[i] = obj;
            Object** temp = (Object**)realloc(objects, (num_objects + 1) * sizeof(Object*));
            if (temp) {
                objects = temp;
                objects[num_objects] = obj;
                num_objects++;
            }
        }
    }

    // Load monsters
    uint32_t monster_count;
    fread(&monster_count, sizeof(monster_count), 1, file);
    monster_count = be32toh(monster_count);
    if (monsters) {
        for (int i = 0; i < num_monsters; i++) {
            if (monsters[i] && monsterAt[monsters[i]->y][monsters[i]->x]) {
                monsterAt[monsters[i]->y][monsters[i]->x] = nullptr;
            }
            delete monsters[i];
        }
        free(monsters);
    }
    num_monsters = monster_count;
    monsters = (NPC**)malloc(num_monsters * sizeof(NPC*));
    for (int i = 0; i < num_monsters; i++) {
        int32_t index;
        uint32_t x, y, hitpoints;
        uint8_t alive;
        fread(&index, sizeof(index), 1, file);
        fread(&x, sizeof(x), 1, file);
        fread(&y, sizeof(y), 1, file);
        fread(&hitpoints, sizeof(hitpoints), 1, file);
        fread(&alive, sizeof(alive), 1, file);
        index = be32toh(index);
        x = be32toh(x);
        y = be32toh(y);
        hitpoints = be32toh(hitpoints);
        if (index >= 0 && index < (int32_t)monsterDescs.size()) {
            NPC* npc = monsterDescs[index].createNPC(x, y);
            npc->hitpoints = hitpoints;
            npc->alive = alive;
            monsters[i] = npc;
            if (alive) {
                monsterAt[y][x] = npc;
            }
        }
    }

    player->recalculate_stats();
    dungeon[player->y][player->x] = '@';
    terrain[player->y][player->x] = '@';

    memset(visible, 0, sizeof(visible));
    memset(remembered, 0, sizeof(remembered));
    update_visibility();

    printf("Dungeon loaded from %s\n", dungeonFile);
    fclose(file);
    delete[] dungeonFile;
    dungeonFile = nullptr;
}