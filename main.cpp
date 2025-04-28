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
    if (monster->damage.base == 0 && monster->damage.dice == 0) {
        mvwprintw(win, 5, 0, "Damage: No damage");
    } else {
        mvwprintw(win, 5, 0, "Damage: %s", monster->damage.toString().c_str());
    }
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
    if (monster->is_boss) mvwprintw(win, line++, 2, "- Boss");

    mvwprintw(win, line++, 0, "Description:");
    for (const auto& desc_line : monsterDescs) {
        if (desc_line.name == monster->name) {
            for (const auto& line_text : desc_line.description) {
                if (line < 23) {
                    mvwprintw(win, line++, 2, "%s", line_text.c_str());
                }
            }
            break;
        }
    }

    keypad(win, TRUE);
    wrefresh(win);
    flushinp();
    getch();
    *message = "Monster info displayed";
}

int main(int argc, char* argv[]) {
    srand(time(NULL));
    int numMonsters = 10; // Default number of monsters as per assignment
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
        } else if (strcmp(argv[i], "--nummon") == 0) {
            if (i + 1 < argc) numMonsters = atoi(argv[++i]);
        }
    }

    loadDescriptions();

    if (parseMonstersOnly || parseObjectsOnly) {
        char* home = getenv("HOME");
        if (!home) {
            fprintf(stderr, "Error: HOME environment variable not set\n");
            return 1;
        }
        if (parseMonstersOnly) {
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

    // Schedule initial events
    while (!event_queue.empty()) event_queue.pop();
    schedule_event(player);
    for (int i = 0; i < num_monsters; i++) {
        if (monsters[i] && monsters[i]->alive) {
            schedule_event(monsters[i]);
        }
    }

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

    message = "Welcome to the dungeon! Press ? for help.";
    draw_dungeon(win, message);
    update_visibility(); // Ensure initial fog state is applied

    bool game_running = true;
    bool teleport_mode = false;
    bool look_mode = false;
    bool targeting_mode = false;
    enum TargetingAction { NONE, RANGED_ATTACK, POISON_BALL } targeting_action = NONE;
    int target_x = player->x, target_y = player->y;

    while (game_running) {
        bool ui_action = false; // Track UI actions to skip monster moves
        int ch = getch();
        // Debug input
        FILE* debug_file = fopen("main_input_debug.txt", "a");
        if (debug_file) {
            fprintf(debug_file, "Main loop: Key pressed: %d ('%c')\n", ch, (char)ch);
            fclose(debug_file);
        }
        int moved = 0;
        int dx = 0, dy = 0;

        if (in_combat) {
            int result = fight_monster(win, engaged_monster, ch, &message);
            if (result == -1) { // Game win
                game_running = false;
            } else if (result == -2) { // Game over
                game_running = false;
            } else if (!in_combat) { // Combat ended
                schedule_event(player);
                draw_dungeon(win, message);
            }
            continue;
        }

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
                    schedule_event(player);
                } else {
                    message = "Cannot teleport into immutable rock!";
                }
                teleport_mode = false;
                moved = 1;
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
                schedule_event(player);
                teleport_mode = false;
                moved = 1;
            } else if (ch == 27) {
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
            } else if (ch == 27) {
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
        } else if (targeting_mode) {
            if (ch == 't') {
                if (targeting_action == RANGED_ATTACK) {
                    moved = fire_ranged_weapon(target_x, target_y, &message);
                } else if (targeting_action == POISON_BALL) {
                    moved = cast_poison_ball(target_x, target_y, &message);
                }
                targeting_mode = false;
                targeting_action = NONE;
                if (moved == -1) { // Game win
                    game_running = false;
                }
            } else if (ch == 27) {
                targeting_mode = false;
                targeting_action = NONE;
                message = "Targeting mode cancelled";
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
                    draw_dungeon(win, targeting_action == RANGED_ATTACK ?
                        "Ranged attack: Move cursor with movement keys, 't' to fire, ESC to cancel" :
                        "Poison Ball: Move cursor with movement keys, 't' to cast, ESC to cancel");
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
                std::priority_queue<Event, std::vector<Event>, std::greater<Event>> temp_queue;
                moved = move_player(dx, dy, &message);
                if (moved) {
                    // Remove player’s current event and schedule a new one
                    while (!event_queue.empty()) {
                        Event e = event_queue.top();
                        event_queue.pop();
                        if (e.character != player) {
                            temp_queue.push(e);
                        }
                    }
                    event_queue = temp_queue;
                    schedule_event(player);
                }
            } else {
                flushinp(); // Clear input buffer before UI prompts
                switch (ch) {
                    case '>':
                    {
                        std::priority_queue<Event, std::vector<Event>, std::greater<Event>> temp_queue;
                        moved = use_stairs('>', numMonsters, &message);
                        if (moved) {
                            // Clear and reschedule events after level change
                            while (!event_queue.empty()) event_queue.pop();
                            schedule_event(player);
                            for (int i = 0; i < num_monsters; i++) {
                                if (monsters[i] && monsters[i]->alive) {
                                    schedule_event(monsters[i]);
                                }
                            }
                        }
                        break;
                    }
                    case '<':
                    {
                        std::priority_queue<Event, std::vector<Event>, std::greater<Event>> temp_queue;
                        moved = use_stairs('<', numMonsters, &message);
                        if (moved) {
                            // Clear and reschedule events after level change
                            while (!event_queue.empty()) event_queue.pop();
                            schedule_event(player);
                            for (int i = 0; i < num_monsters; i++) {
                                if (monsters[i] && monsters[i]->alive) {
                                    schedule_event(monsters[i]);
                                }
                            }
                        }
                        break;
                    }
                    case '5': case ' ': case '.':
                    {
                        std::priority_queue<Event, std::vector<Event>, std::greater<Event>> temp_queue;
                        moved = 1;
                        message = "Resting...";
                        // Treat rest as a move for event scheduling
                        while (!event_queue.empty()) {
                            Event e = event_queue.top();
                            event_queue.pop();
                            if (e.character != player) {
                                temp_queue.push(e);
                            }
                        }
                        event_queue = temp_queue;
                        schedule_event(player);
                        break;
                    }
                    case 'm':
                        draw_monster_list(win);
                        message = "";
                        ui_action = true;
                        break;
                    case 'f':
                        fog_enabled = !fog_enabled;
                        message = fog_enabled ? "Fog of War ON" : "Fog of War OFF";
                        update_visibility(); // Update visibility to reflect new fog state
                        draw_dungeon(win, message); // Redraw immediately
                        ui_action = true;
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
                    case 'r':
                        targeting_mode = true;
                        targeting_action = RANGED_ATTACK;
                        target_x = player->x;
                        target_y = player->y;
                        message = "Ranged attack: Move cursor with movement keys, 't' to fire, ESC to cancel";
                        break;
                    case 'p':
                        targeting_mode = true;
                        targeting_action = POISON_BALL;
                        target_x = player->x;
                        target_y = player->y;
                        message = "Poison Ball: Move cursor with movement keys, 't' to cast, ESC to cancel";
                        break;
                    case 'w':
                        wear_item(win, player, &message);
                        ui_action = true;
                        break;
                    case 't':
                        take_off_item(win, player, &message);
                        ui_action = true;
                        break;
                    case 'd':
                        drop_item(win, player, &message);
                        ui_action = true;
                        break;
                    case 'x':
                        expunge_item(win, player, &message);
                        ui_action = true;
                        break;
                    case 'i':
                        display_inventory(win, player, &message);
                        ui_action = true;
                        break;
                    case 'e':
                        display_equipment(win, player, &message);
                        ui_action = true;
                        break;
                    case 's':
                        display_stats(win, player, &message);
                        ui_action = true;
                        break;
                    case '?':
                        display_help(win, &message);
                        ui_action = true;
                        break;
                    case 'I':
                        inspect_item(win, player, &message);
                        ui_action = true;
                        break;
                    case 'q': case 'Q':
                        game_running = false;
                        message = "Quitting game...";
                        break;
                    default:
                        message = "Unknown command";
                        break;
                }
            }
        }

        // Process monster events only after a player action, excluding UI interactions
        if (moved && game_running && !teleport_mode && !look_mode && !targeting_mode && !in_combat && !ui_action) {
            // Advance game_turn by player's turn duration
            int total_speed;
            Dice total_damage;
            int total_defense, total_hit, total_dodge;
            player->calculateStats(total_speed, total_damage, total_defense, total_hit, total_dodge);
            int64_t player_turn_duration = 1000 / total_speed; // Use total speed including equipment
            game_turn += player_turn_duration;

            // Process all events up to the new game_turn
            std::priority_queue<Event, std::vector<Event>, std::greater<Event>> temp_queue;
            while (!event_queue.empty() && event_queue.top().time <= game_turn) {
                Event event = event_queue.top();
                event_queue.pop();
                if (event.character->alive && event.character != player) {
                    event.character->move();
                    int64_t delay = 1000 / event.character->speed;
                    int64_t next_time = event.time + delay;
                    event_queue.emplace(next_time, event.character, Event::MOVE);
                }
            }

            // Requeue any events that were not processed
            while (!event_queue.empty()) {
                Event e = event_queue.top();
                event_queue.pop();
                temp_queue.push(e);
            }
            event_queue = temp_queue;

            if (!player->alive) {
                message = "You were killed! Game over!";
                game_running = false;
            }

            draw_dungeon(win, message);
        } else if (!moved && !teleport_mode && !look_mode && !targeting_mode && !in_combat) {
            // Redraw dungeon for UI actions or invalid commands
            draw_dungeon(win, message);
        }
    }

    // Display final message before exiting
    if (!in_combat) {
        draw_dungeon(win, message);
    }
    sleep(2);

    // Cleanup
    if (save && saveFileName) {
        saveDungeon(saveFileName);
    }

    for (int i = 0; i < num_monsters; i++) {
        if (monsters[i]) {
            delete monsters[i];
        }
    }
    free(monsters);
    cleanupObjects();
    delete player;
    delwin(win);
    endwin();
    return 0;
}