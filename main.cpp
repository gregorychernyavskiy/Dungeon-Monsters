#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <ctype.h>
#include <ncurses.h>
#include "dungeon_generation.h"
#include "minheap.h"

int main(int argc, char* argv[]) {
    srand(time(NULL));
    int numMonsters = 0;
    char* saveFileName = nullptr;
    int save = 0;
    bool parseMonstersOnly = false;
    bool parseObjectsOnly = false;

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--save") == 0) {
            save = 1;
            if (i + 1 < argc) saveFileName = argv[++i];
        } else if (strcmp(argv[i], "--parse-monsters") == 0) {
            parseMonstersOnly = true;
        } else if (strcmp(argv[i], "--parse-objects") == 0) {
            parseObjectsOnly = true;
        } else {
            numMonsters = atoi(argv[i]);
        }
    }

    loadDescriptions();

    if (argc == 1 || parseMonstersOnly || parseObjectsOnly) {
        char* home = getenv("HOME");
        if (!home) {
            fprintf(stderr, "Error: HOME environment variable not set\n");
            return 1;
        }
        if (parseMonstersOnly || (argc == 1 && !parseObjectsOnly)) {
            std::string filename = std::string(home) + "/.rlg327/monster_desc.txt";
            std::vector<MonsterDescription> monsters = parseMonsterDescriptions(filename);
            for (const auto& monster : monsters) {
                monster.print();
            }
        }
        if (parseObjectsOnly) {
            std::string filename = std::string(home) + "/.rlg327/object_desc.txt";
            std::vector<ObjectDescription> objects = parseObjectDescriptions(filename);
            for (const auto& object : objects) {
                object.print();
            }
        }
        return 0;
    }

    emptyDungeon();
    createRooms();
    connectRooms();
    placeStairs();
    memcpy(terrain, dungeon, sizeof(dungeon));
    placePlayer();
    initializeHardness();
    if (numMonsters > 0) spawnMonsters(numMonsters);
    placeObjects(10);
    update_visibility();

    if (save && saveFileName && numMonsters == 0 && argc == 3) {
        saveDungeon(saveFileName);
        for (int i = 0; i < num_monsters; i++) delete monsters[i];
        free(monsters);
        cleanupObjects();
        delete player;
        return 0;
    }

    init_ncurses();
    WINDOW* win = newwin(24, 80, 0, 0);
    if (!win) {
        endwin();
        fprintf(stderr, "Error: Failed to create window\n");
        return 1;
    }

    const char* message = "Press any button to start";
    draw_dungeon(win, message);
    getch();

    message = "Welcome to the dungeon!";
    draw_dungeon(win, message);

    bool game_running = true;
    bool teleport_mode = false;
    bool inspect_mode = false;
    int target_x = player->x, target_y = player->y;

    while (game_running) {
        fprintf(stderr, "Game loop: teleport_mode=%d, inspect_mode=%d\n", teleport_mode, inspect_mode);
        int ch = getch();
        fprintf(stderr, "Input received: %c (%d)\n", ch, ch);
        int moved = 0;
        int dx = 0, dy = 0;

        if (teleport_mode || inspect_mode) {
            fprintf(stderr, "In %s mode\n", teleport_mode ? "teleport" : "inspect");
            if (inspect_mode && ch == 't' && monsterAt[target_y][target_x] && visible[target_y][target_x]) {
                inspect_monster(win, target_x, target_y);
                inspect_mode = false;
                message = "";
            } else if (teleport_mode && ch == 'g') {
                if (hardness[target_y][target_x] != 255) {
                    dungeon[player->y][player->x] = terrain[player->y][player->x];
                    if (objectAt[player->y][player->x]) {
                        terrain[player->y][player->x] = objectAt[player->y][player->x]->symbol;
                    }
                    player->x = target_x;
                    player->y = target_y;
                    dungeon[player->y][player->x] = '@';
                    update_visibility();
                    message = "Teleported!";
                } else {
                    message = "Cannot teleport into immutable rock!";
                }
                teleport_mode = false;
            } else if (teleport_mode && ch == 'r') {
                int new_x, new_y;
                do {
                    new_x = rand() % WIDTH;
                    new_y = rand() % HEIGHT;
                } while (hardness[new_y][new_x] == 255);
                dungeon[player->y][player->x] = terrain[player->y][player->x];
                if (objectAt[player->y][player->x]) {
                    terrain[player->y][player->x] = objectAt[player->y][player->x]->symbol;
                }
                player->x = new_x;
                player->y = new_y;
                dungeon[player->y][player->x] = '@';
                update_visibility();
                message = "Teleported to random location!";
                teleport_mode = false;
            } else if (ch == 27) {
                teleport_mode = false;
                inspect_mode = false;
                message = "";
            } else if (ch == '7' || ch == 'y') { dx = -1; dy = -1; }
            else if (ch == '8' || ch == 'k') { dx = 0; dy = -1; }
            else if (ch == '9' || ch == 'u') { dx = 1; dy = -1; }
            else if (ch == '6' || ch == 'l') { dx = 1; dy = 0; }
            else if (ch == '3' || ch == 'n') { dx = 1; dy = 1; }
            else if (ch == '2' || ch == 'j') { dx = 0; dy = 1; }
            else if (ch == '1' || ch == 'b') { dx = -1; dy = 1; }
            else if (ch == '4' || ch == 'h') { dx = -1; dy = 0; }

            if (dx != 0 || dy != 0) {
                int new_tx = target_x + dx, new_ty = target_y + dy;
                if (new_tx >= 0 && new_tx < WIDTH && new_ty >= 0 && new_ty < HEIGHT) {
                    target_x = new_tx;
                    target_y = new_ty;
                    werase(win);
                    draw_dungeon(win, inspect_mode ? "Inspect mode: Move cursor, 't' to select, ESC to abort" :
                                                     "Teleport mode: Move cursor, 'g' to confirm, 'r' for random");
                    mvwprintw(win, target_y + 1, target_x, "*");
                    wrefresh(win);
                }
            }
        } else {
            if (ch == '7' || ch == 'y') { dx = -1; dy = -1; }
            else if (ch == '8' || ch == 'k') { dx = 0; dy = -1; }
            else if (ch == '9' || ch == 'u') { dx = 1; dy = -1; }
            else if (ch == '6' || ch == 'l') { dx = 1; dy = 0; }
            else if (ch == '3' || ch == 'n') { dx = 1; dy = 1; }
            else if (ch == '2' || ch == 'j') { dx = 0; dy = 1; }
            else if (ch == '1' || ch == 'b') { dx = -1; dy = 1; }
            else if (ch == '4' || ch == 'h') { dx = -1; dy = 0; }
            else if (ch == 'w') {
                werase(win);
                mvwprintw(win, 0, 0, "Wear item (0-9 or ESC):");
                for (int i = 0; i < 10; i++) {
                    if (player->carry[i]) {
                        mvwprintw(win, i + 1, 0, "%d: %s", i, player->carry[i]->name.c_str());
                    }
                }
                wrefresh(win);
                ch = getch();
                if (ch == 27) {
                    message = "Wear cancelled";
                } else if (ch >= '0' && ch <= '9') {
                    int slot = ch - '0';
                    if (player->carry[slot]) {
                        Object* obj = player->carry[slot];
                        int equip_slot = -1;
                        if (obj->types[0] == "WEAPON") equip_slot = 0;
                        else if (obj->types[0] == "OFFHAND") equip_slot = 1;
                        else if (obj->types[0] == "RANGED") equip_slot = 2;
                        else if (obj->types[0] == "ARMOR") equip_slot = 3;
                        else if (obj->types[0] == "HELMET") equip_slot = 4;
                        else if (obj->types[0] == "CLOAK") equip_slot = 5;
                        else if (obj->types[0] == "GLOVES") equip_slot = 6;
                        else if (obj->types[0] == "BOOTS") equip_slot = 7;
                        else if (obj->types[0] == "AMULET") equip_slot = 8;
                        else if (obj->types[0] == "LIGHT") equip_slot = 9;
                        else if (obj->types[0] == "RING") {
                            equip_slot = player->equipment[10] ? 11 : 10;
                        }
                        if (equip_slot >= 0) {
                            if (player->equipment[equip_slot]) {
                                Object* temp = player->equipment[equip_slot];
                                player->equipment[equip_slot] = obj;
                                player->carry[slot] = temp;
                            } else {
                                player->equipment[equip_slot] = obj;
                                player->carry[slot] = nullptr;
                            }
                            player->recalculate_stats();
                            message = "Item equipped!";
                        } else {
                            message = "Invalid item type!";
                        }
                    } else {
                        message = "No item in that slot!";
                    }
                }
            } else if (ch == 't') {
                werase(win);
                mvwprintw(win, 0, 0, "Take off item (a-l or ESC):");
                const char* slots[] = {"WEAPON", "OFFHAND", "RANGED", "ARMOR", "HELMET", "CLOAK",
                                       "GLOVES", "BOOTS", "AMULET", "LIGHT", "RING1", "RING2"};
                for (int i = 0; i < 12; i++) {
                    mvwprintw(win, i + 1, 0, "%c: %s%s", 'a' + i, slots[i], 
                              player->equipment[i] ? (" (" + player->equipment[i]->name + ")").c_str() : "");
                }
                wrefresh(win);
                ch = getch();
                if (ch == 27) {
                    message = "Take off cancelled";
                } else if (ch >= 'a' && ch <= 'l') {
                    int slot = ch - 'a';
                    if (player->equipment[slot]) {
                        bool placed = false;
                        for (int i = 0; i < 10; i++) {
                            if (!player->carry[i]) {
                                player->carry[i] = player->equipment[slot];
                                player->equipment[slot] = nullptr;
                                placed = true;
                                break;
                            }
                        }
                        if (placed) {
                            player->recalculate_stats();
                            message = "Item taken off!";
                        } else {
                            message = "No free carry slots!";
                        }
                    } else {
                        message = "No item in that slot!";
                    }
                }
            } else if (ch == 'd') {
                werase(win);
                mvwprintw(win, 0, 0, "Drop item (0-9 or ESC):");
                for (int i = 0; i < 10; i++) {
                    if (player->carry[i]) {
                        mvwprintw(win, i + 1, 0, "%d: %s", i, player->carry[i]->name.c_str());
                    }
                }
                wrefresh(win);
                ch = getch();
                if (ch == 27) {
                    message = "Drop cancelled";
                } else if (ch >= '0' && ch <= '9') {
                    int slot = ch - '0';
                    if (player->carry[slot]) {
                        Object* obj = player->carry[slot];
                        obj->x = player->x;
                        obj->y = player->y;
                        objectAt[obj->y][obj->x] = obj;
                        terrain[obj->y][obj->x] = obj->symbol;
                        player->carry[slot] = nullptr;
                        Object** temp = (Object**)realloc(objects, (num_objects + 1) * sizeof(Object*));
                        if (temp) {
                            objects = temp;
                            objects[num_objects] = obj;
                            num_objects++;
                        }
                        message = "Item dropped!";
                    } else {
                        message = "No item in that slot!";
                    }
                }
            } else if (ch == 'x') {
                werase(win);
                mvwprintw(win, 0, 0, "Expunge item (0-9 or ESC):");
                for (int i = 0; i < 10; i++) {
                    if (player->carry[i]) {
                        mvwprintw(win, i + 1, 0, "%d: %s", i, player->carry[i]->name.c_str());
                    }
                }
                wrefresh(win);
                ch = getch();
                if (ch == 27) {
                    message = "Expunge cancelled";
                } else if (ch >= '0' && ch <= '9') {
                    int slot = ch - '0';
                    if (player->carry[slot]) {
                        delete player->carry[slot];
                        player->carry[slot] = nullptr;
                        message = "Item expunged!";
                    } else {
                        message = "No item in that slot!";
                    }
                }
            } else if (ch == 'i') {
                werase(win);
                mvwprintw(win, 0, 0, "Inventory (Press any key to exit):");
                for (int i = 0; i < 10; i++) {
                    if (player->carry[i]) {
                        mvwprintw(win, i + 1, 0, "%d: %s", i, player->carry[i]->name.c_str());
                    } else {
                        mvwprintw(win, i + 1, 0, "%d: Empty", i);
                    }
                }
                wrefresh(win);
                getch();
                message = "";
            } else if (ch == 'e') {
                werase(win);
                mvwprintw(win, 0, 0, "Equipment (Press any key to exit):");
                const char* slots[] = {"WEAPON", "OFFHAND", "RANGED", "ARMOR", "HELMET", "CLOAK",
                                       "GLOVES", "BOOTS", "AMULET", "LIGHT", "RING1", "RING2"};
                for (int i = 0; i < 12; i++) {
                    if (player->equipment[i]) {
                        mvwprintw(win, i + 1, 0, "%c: %s", 'a' + i, player->equipment[i]->name.c_str());
                    } else {
                        mvwprintw(win, i + 1, 0, "%c: Empty (%s)", 'a' + i, slots[i]);
                    }
                }
                wrefresh(win);
                getch();
                message = "";
            } else if (ch == 'I') {
                werase(win);
                mvwprintw(win, 0, 0, "Inspect item (0-9 or ESC):");
                for (int i = 0; i < 10; i++) {
                    if (player->carry[i]) {
                        mvwprintw(win, i + 1, 0, "%d: %s", i, player->carry[i]->name.c_str());
                    }
                }
                wrefresh(win);
                ch = getch();
                if (ch == 27) {
                    message = "Inspect cancelled";
                } else if (ch >= '0' && ch <= '9') {
                    int slot = ch - '0';
                    if (player->carry[slot]) {
                        Object* obj = player->carry[slot];
                        werase(win);
                        mvwprintw(win, 0, 0, "Item: %s", obj->name.c_str());
                        for (size_t i = 0; i < objectDescs.size(); i++) {
                            if (objectDescs[i].name == obj->name) {
                                for (size_t j = 0; j < objectDescs[i].description.size(); j++) {
                                    mvwprintw(win, j + 1, 0, "%s", objectDescs[i].description[j].c_str());
                                }
                                mvwprintw(win, objectDescs[i].description.size() + 1, 0, "Type: %s", obj->types[0].c_str());
                                mvwprintw(win, objectDescs[i].description.size() + 2, 0, "Damage: %s", objectDescs[i].damage.toString().c_str());
                                break;
                            }
                        }
                        wrefresh(win);
                        getch();
                        message = "";
                    } else {
                        message = "No item in that slot!";
                    }
                }
            } else if (ch == 'L') {
                inspect_mode = true;
                target_x = player->x;
                target_y = player->y;
                message = "Inspect mode: Move cursor, 't' to select, ESC to abort";
            } else if (dx != 0 || dy != 0) {
                moved = move_player(dx, dy, &message);
            } else {
                switch (ch) {
                    case '>':
                        moved = use_stairs('>', numMonsters, &message);
                        break;
                    case '<':
                        moved = use_stairs('<', numMonsters, &message);
                        break;
                    case '5': case ' ': case '.':
                        moved = 1;
                        message = "Resting...";
                        break;
                    case 'm':
                        draw_monster_list(win);
                        message = "";
                        break;
                    case 'f':
                        fog_enabled = !fog_enabled;
                        message = fog_enabled ? "Fog of War ON" : "Fog of War OFF";
                        break;
                    case 'g':
                        teleport_mode = true;
                        target_x = player->x;
                        target_y = player->y;
                        message = "Teleport mode: Move cursor with movement keys, 'g' to confirm, 'r' for random";
                        break;
                    case 'Q': case 'q':
                        game_running = false;
                        message = "Quitting game...";
                        break;
                    default:
                        message = "Unknown command";
                        break;
                }
            }
        }

        if (moved && game_running && !teleport_mode && !inspect_mode) {
            fprintf(stderr, "Processing NPC moves\n");
            for (int i = 0; i < num_monsters; i++) {
                if (monsters[i]->alive) {
                    monsters[i]->move();
                }
            }
            NPC* culprit = nullptr;
            bool boss_killed = false;
            if (gameOver(&culprit, &boss_killed)) {
                char buf[80];
                if (player->hitpoints <= 0) {
                    snprintf(buf, sizeof(buf), "Killed by %s!", culprit ? culprit->name.c_str() : "an unknown foe");
                    message = buf;
                } else if (boss_killed) {
                    snprintf(buf, sizeof(buf), "You defeated SpongeBob SquarePants! Victory!");
                    message = buf;
                }
                game_running = false;
            }
        }

        if (!teleport_mode && !inspect_mode) {
            draw_dungeon(win, message);
        }
    }

    draw_dungeon(win, message);
    sleep(2);

    if (save && saveFileName) {
        saveDungeon(saveFileName);
    }

    for (int i = 0; i < num_monsters; i++) {
        delete monsters[i];
    }
    free(monsters);
    cleanupObjects();
    delete player;
    delwin(win);
    endwin();
    return 0;
}