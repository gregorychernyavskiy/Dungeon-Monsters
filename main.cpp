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

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--save") == 0) {
            save = 1;
            if (i + 1 < argc) saveFileName = argv[++i];
        } else {
            numMonsters = atoi(argv[i]);
        }
    }

    // Generate the dungeon state regardless of save/load intent
    emptyDungeon();
    createRooms();
    connectRooms();
    placeStairs();
    memcpy(terrain, dungeon, sizeof(dungeon));
    placePlayer();
    initializeHardness();
    if (numMonsters > 0) spawnMonsters(numMonsters);
    update_visibility();

    // If save is requested and no gameplay is implied, save and exit
    if (save && saveFileName && numMonsters == 0 && argc == 3) { // Only --save <filename>
        saveDungeon(saveFileName);
        for (int i = 0; i < num_monsters; i++) delete monsters[i];
        free(monsters);
        delete player;
        return 0;
    }

    // Proceed to game loop if not just saving
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
    int target_x = player->x, target_y = player->y;

    while (game_running) {
        int ch = getch();
        int moved = 0;
        int dx = 0, dy = 0;

        if (teleport_mode) {
            if (ch == 'g') {
                if (hardness[target_y][target_x] != 255) {
                    dungeon[player->y][player->x] = terrain[player->y][player->x];
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
                player->x = new_x;
                player->y = new_y;
                dungeon[player->y][player->x] = '@';
                update_visibility();
                message = "Teleported to random location!";
                teleport_mode = false;
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

        if (moved && game_running && !teleport_mode) {
            for (int i = 0; i < num_monsters; i++) {
                if (monsters[i]->alive) {
                    monsters[i]->move();
                }
            }
            NPC* culprit = nullptr;
            if (gameOver(&culprit)) {
                int personality = culprit->intelligent + (culprit->telepathic << 1) +
                                  (culprit->tunneling << 2) + (culprit->erratic << 3);
                char symbol = personality < 10 ? '0' + personality : 'A' + (personality - 10);
                char buf[80];
                snprintf(buf, sizeof(buf), "Killed by monster '%c'!", symbol);
                message = buf;
                game_running = false;
            }
        }

        if (!teleport_mode) {
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
    delete player;
    delwin(win);
    endwin();
    return 0;
}