#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <ctype.h>
#include <ncurses.h>
#include "dungeon_generation.h"
#include "minheap.h"

void display_monster_info(WINDOW* win, NPC* monster, const char** message) {
    if (!monster) {
        *message = "No monster at this location!";
        return;
    }
    if (fog_enabled && !visible[monster->y][monster->x]) {
        *message = "Monster is not visible!";
        return;
    }

    werase(win);
    mvwprintw(win, 0, 0, "Monster Information (Press any key to exit):");
    mvwprintw(win, 1, 0, "Name: %s", monster->name.c_str());
    mvwprintw(win, 2, 0, "Symbol: %c", monster->symbol);
    mvwprintw(win, 3, 0, "Position: (%d, %d)", monster->x, monster->y);
    mvwprintw(win, 4, 0, "Hitpoints: %d", monster->hitpoints);
    mvwprintw(win, 5, 0, "Damage: %s", monster->damage.toString().c_str());
    mvwprintw(win, 6, 0, "Speed: %d", monster->speed);
    mvwprintw(win, 7, 0, "Abilities:");
    int line = 8;
    if (monster->intelligent) mvwprintw(win, line++, 2, "- Intelligent");
    if (monster->telepathic) mvwprintw(win, line++, 2, "- Telepathic");
    if (monster->tunneling) mvwprintw(win, line++, 2, "- Tunneling");
    if (monster->erratic) mvwprintw(win, line++, 2, "- Erratic");
    if (monster->pass_wall) mvwprintw(win, line++, 2, "- Pass Wall");
    if (monster->pickup) mvwprintw(win, line++, 2, "- Pickup");
    if (monster->destroy) mvwprintw(win, line++, 2, "- Destroy");
    if (monster->is_unique) mvwprintw(win, line++, 2, "- Unique");

    mvwprintw(win, line++, 0, "Description:");
    for (const auto& desc_line : monsterDescs) {
        if (desc_line.name == monster->name) {
            for (const auto& line_text : desc_line.description) {
                if (line < 23) { // Prevent overflow on window
                    mvwprintw(win, line++, 2, "%s", line_text.c_str());
                }
            }
            break;
        }
    }

    wrefresh(win);
    getch();
    *message = "Monster info displayed";
}

int main(int argc, char* argv[]) {
    srand(time(NULL));
    int numMonsters = 0;
    char* saveFileName = nullptr;
    int save = 0;
    bool parseMonstersOnly = false;
    bool parseObjectsOnly = false;

    // Parse command-line arguments
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

    // Load descriptions
    loadDescriptions();

    // If no arguments or parsing mode is specified
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

    // Original dungeon generation and game logic
    emptyDungeon();
    createRooms();
    connectRooms();
    placeStairs();
    memcpy(terrain, dungeon, sizeof(dungeon));
    placePlayer();
    initializeHardness();
    if (numMonsters > 0) spawnMonsters(numMonsters);
    placeObjects(10); // At least 10 objects
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
    bool look_mode = false;
    int target_x = player->x, target_y = player->y;

    while (game_running) {
        int ch = getch();
        int moved = 0;
        int dx = 0, dy = 0;

        if (teleport_mode) {
            if (ch == 'g') {
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
            } else if (ch == 'r') {
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
            } else if (ch == 27) { // Escape to exit teleport mode
                teleport_mode = false;
                message = "Teleport mode cancelled";
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
                    draw_dungeon(win, "Teleport mode: Move cursor with movement keys, 'g' to confirm, 'r' for random");
                    mvwprintw(win, target_y + 1, target_x, "*");
                    wrefresh(win);
                }
            }
        } else if (look_mode) {
            if (ch == 't') {
                display_monster_info(win, monsterAt[target_y][target_x], &message);
                look_mode = false;
            } else if (ch == 27) { // Escape to exit look mode
                look_mode = false;
                message = "Look mode cancelled";
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
                    draw_dungeon(win, "Look mode: Move cursor with movement keys, 't' to select monster, ESC to exit");
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

            if (dx != 0 || dy != 0) {
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
                    case 'L':
                        look_mode = true;
                        target_x = player->x;
                        target_y = player->y;
                        message = "Look mode: Move cursor with movement keys, 't' to select monster, ESC to exit";
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

        if (moved && game_running && !teleport_mode && !look_mode) {
            for (int i = 0; i < num_monsters; i++) {
                if (monsters[i]->alive) {
                    monsters[i]->move();
                }
            }
            NPC* culprit = nullptr;
            if (gameOver(&culprit)) {
                char buf[80];
                snprintf(buf, sizeof(buf), "Killed by monster '%s'!", culprit->name.c_str());
                message = buf;
                game_running = false;
            }
        }

        if (!teleport_mode && !look_mode) {
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