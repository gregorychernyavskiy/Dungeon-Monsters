#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <ctype.h>
#include <ncurses.h>
#include "dungeon_generation.h"
#include "minheap.h"

void init_ncurses() {
    initscr();
    raw();           // Unbuffered input
    noecho();        // Don't echo input
    keypad(stdscr, TRUE); // Enable arrow keys
    curs_set(0);     // Hide cursor
}

void draw_dungeon(WINDOW *win, const char *message) {
    werase(win);
    // Message line
    mvwprintw(win, 0, 0, "%s", message ? message : "");

    // Dungeon (lines 1-21, zero-indexed)
    for (int y = 0; y < HEIGHT; y++) {
        for (int x = 0; x < WIDTH; x++) {
            if (monsterAt[y][x]) {
                int personality = monsterAt[y][x]->intelligent +
                                  (monsterAt[y][x]->telepathic << 1) +
                                  (monsterAt[y][x]->tunneling << 2) +
                                  (monsterAt[y][x]->erratic << 3);
                char symbol = personality < 10 ? '0' + personality : 'A' + (personality - 10);
                mvwprintw(win, y + 1, x, "%c", symbol);
            } else if (x == player_x && y == player_y) {
                mvwprintw(win, y + 1, x, "@");
            } else {
                mvwprintw(win, y + 1, x, "%c", dungeon[y][x]);
            }
        }
    }
    // Status lines (22-23) - Placeholder for future use
    mvwprintw(win, 22, 0, "Status Line 1");
    mvwprintw(win, 23, 0, "Status Line 2");
    wrefresh(win);
}

void draw_monster_list(WINDOW *win) {
    werase(win);
    int start = 0; // For scrolling
    int max_lines = 22; // Lines 0-21 available, zero-indexed
    int ch;

    while (1) {
        werase(win);
        mvwprintw(win, 0, 0, "Monster List (Esc to exit):");
        for (int i = start; i < num_monsters && i - start < max_lines - 1; i++) {
            if (monsters[i]->alive) {
                int dx = monsters[i]->x - player_x;
                int dy = monsters[i]->y - player_y;
                int personality = monsters[i]->intelligent +
                                  (monsters[i]->telepathic << 1) +
                                  (monsters[i]->tunneling << 2) +
                                  (monsters[i]->erratic << 3);
                char symbol = personality < 10 ? '0' + personality : 'A' + (personality - 10);
                const char *ns = dy < 0 ? "north" : "south";
                const char *ew = dx < 0 ? "west" : "east";
                mvwprintw(win, i - start + 1, 0, "%c, %d %s and %d %s", 
                          symbol, abs(dy), ns, abs(dx), ew);
            }
        }
        wrefresh(win);

        ch = getch();
        if (ch == 27) { // Escape
            break;
        } else if (ch == KEY_UP && start > 0) {
            start--;
        } else if (ch == KEY_DOWN && start + max_lines - 1 < num_monsters) {
            start++;
        }
    }
}

void regenerate_dungeon(int numMonsters) {
    // Free existing monsters
    for (int i = 0; i < num_monsters; i++) {
        if (monsters[i]) {
            if (monsterAt[monsters[i]->y][monsters[i]->x] == monsters[i]) {
                monsterAt[monsters[i]->y][monsters[i]->x] = NULL;
            }
            free(monsters[i]);
        }
    }
    free(monsters);
    monsters = NULL;
    num_monsters = 0;

    // Regenerate dungeon
    emptyDungeon();
    createRooms();
    connectRooms();
    placeStairs();
    placePlayer();
    initializeHardness();
    spawnMonsters(numMonsters);
}

int move_player(int dx, int dy, const char **message) {
    int new_x = player_x + dx;
    int new_y = player_y + dy;
    if (new_x < 0 || new_x >= WIDTH || new_y < 0 || new_y >= HEIGHT) {
        *message = "Edge of dungeon!";
        return 0;
    }
    if (hardness[new_y][new_x] > 0) {
        *message = "There's a wall in the way!";
        return 0;
    }
    if (monsterAt[new_y][new_x]) {
        *message = "A monster blocks your path!";
        return 0;
    }
    char orig = dungeon[player_y][player_x];
    dungeon[player_y][player_x] = (orig == '<' || orig == '>') ? orig : (player_room_index >= 0 ? '.' : '#');
    player_x = new_x;
    player_y = new_y;
    dungeon[player_y][player_x] = '@';
    *message = "";
    return 1;
}

int use_stairs(char direction, int numMonsters, const char **message) {
    if (direction == '>' && dungeon[player_y][player_x] == '>') {
        regenerate_dungeon(numMonsters);
        *message = "Descended to a new level!";
        return 1;
    } else if (direction == '<' && dungeon[player_y][player_x] == '<') {
        regenerate_dungeon(numMonsters);
        *message = "Ascended to a new level!";
        return 1;
    }
    *message = "Not standing on the correct staircase!";
    return 0;
}

int main(int argc, char *argv[]) {
    srand(time(NULL));
    int numMonsters = 0;
    char *saveFileName = NULL;
    int save = 0;

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--save") == 0) {
            save = 1;
            if (i + 1 < argc) saveFileName = argv[i + 1], i++;
        } else {
            numMonsters = atoi(argv[i]);
        }
    }

    init_ncurses();
    WINDOW *win = newwin(24, 80, 0, 0);

    emptyDungeon();
    createRooms();
    connectRooms();
    placeStairs();
    placePlayer();
    initializeHardness();
    if (numMonsters > 0) spawnMonsters(numMonsters);

    const char *message = "";
    int game_running = 1;

    // Draw initial dungeon state
    draw_dungeon(win, message);
    refresh(); // Ensure the screen updates immediately

    while (game_running) {
        int ch = getch();
        int moved = 0;
        switch (ch) {
            case '7': case 'y': moved = move_player(-1, -1, &message); break;
            case '8': case 'k': moved = move_player(0, -1, &message); break;
            case '9': case 'u': moved = move_player(1, -1, &message); break;
            case '6': case 'l': moved = move_player(1, 0, &message); break;
            case '3': case 'n': moved = move_player(1, 1, &message); break;
            case '2': case 'j': moved = move_player(0, 1, &message); break;
            case '1': case 'b': moved = move_player(-1, 1, &message); break;
            case '4': case 'h': moved = move_player(-1, 0, &message); break;
            case '>': moved = use_stairs('>', numMonsters, &message); break;
            case '<': moved = use_stairs('<', numMonsters, &message); break;
            case '5': case ' ': case '.': moved = 1; message = "Resting..."; break;
            case 'm': draw_monster_list(win); message = ""; break;
            case 'Q': game_running = 0; message = "Quitting game..."; break;
            default: message = "Unknown command"; break;
        }

        if (moved && game_running) {
            for (int i = 0; i < num_monsters; i++) {
                if (monsters[i]->alive) relocateMonster(monsters[i]);
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

    draw_dungeon(win, message); // Final state display
    refresh();
    sleep(2); // Show final message for 2 seconds

    if (save && saveFileName) saveDungeon(saveFileName);

    for (int i = 0; i < num_monsters; i++) {
        if (monsters[i]) free(monsters[i]);
    }
    free(monsters);
    delwin(win);
    endwin();
    return 0;
}