#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <ctype.h>
#include <ncurses.h>
#include "dungeon_generation.h"
#include "minheap.h"

int main(int argc, char *argv[]) {
    srand(time(NULL));
    int numMonsters = 0;
    char *saveFileName = NULL;
    int save = 0;

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--save") == 0) {
            save = 1;
            if (i + 1 < argc) {
                saveFileName = argv[i + 1];
                i++;
            }
        } else {
            numMonsters = atoi(argv[i]);
        }
    }

    init_ncurses();
    WINDOW *win = newwin(24, 80, 0, 0);
    if (!win) {
        endwin();
        fprintf(stderr, "Error: Failed to create window\n");
        return 1;
    }

    player_x = -1;
    player_y = -1;

    memset(visible, 0, sizeof(visible));
    memset(terrain, 0, sizeof(terrain));

    emptyDungeon();
    createRooms();
    connectRooms();
    placeStairs();
    
    for (int y = 0; y < HEIGHT; y++) {
        for (int x = 0; x < WIDTH; x++) {
            terrain[y][x] = dungeon[y][x];
        }
    }
    
    for (int y = 0; y < HEIGHT; y++) {
        for (int x = 0; x < WIDTH; x++) {
            if (dungeon[y][x] == '@') {
                dungeon[y][x] = '.'; // Clear any stray '@'
            }
        }
    }
    placePlayer();
    initializeHardness();
    if (numMonsters > 0) {
        spawnMonsters(numMonsters);
    }
    update_visibility();

    // Display initial welcome message and wait for 'f'
    const char *welcome_message = "Welcome to the dungeon, press f to start";
    draw_dungeon(win, welcome_message);
    wrefresh(win);
    refresh();

    int ch;
    do {
        ch = getch();
    } while (ch != 'f' && ch != 'F');

    // Now start the game with the regular message
    const char *message = "Welcome to the dungeon!";
    draw_dungeon(win, message);
    wrefresh(win);
    refresh();

    int game_running = 1;
    while (game_running) {
        ch = getch();
        int moved = 0;
        int dx = 0, dy = 0;

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
                case 'Q': case 'q':
                    game_running = 0;
                    message = "Quitting game...";
                    break;
                default:
                    message = "Unknown command";
                    break;
            }
        }

        if (moved && game_running) {
            for (int i = 0; i < num_monsters; i++) {
                if (monsters[i]->alive) {
                    relocateMonster(monsters[i]);
                }
            }
            Monster *culprit = NULL;
            if (gameOver(&culprit)) {
                int personality = culprit->intelligent + (culprit->telepathic << 1) +
                                  (culprit->tunneling << 2) + (culprit->erratic << 3);
                char symbol = personality < 10 ? '0' + personality : 'A' + (personality - 10);
                char buf[80];
                snprintf(buf, sizeof(buf), "Killed by monster '%c'!", symbol);
                message = buf;
                game_running = 0;
            }
        }
        draw_dungeon(win, message);
    }

    draw_dungeon(win, message);
    wrefresh(win);
    refresh();
    sleep(2);

    if (save && saveFileName) {
        saveDungeon(saveFileName);
    }

    for (int i = 0; i < num_monsters; i++) {
        if (monsters[i]) {
            free(monsters[i]);
        }
    }
    free(monsters);
    delwin(win);
    endwin();
    return 0;
}