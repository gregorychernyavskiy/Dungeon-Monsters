#include "monster_parsing.h"
#include <fstream>
#include <iostream>
#include <sstream>
#include <cstring>
#include <stdlib.h>
#include <set>
#include <random>
#include "dungeon_generation.h"

MonsterDescription::Monster Queen's monster description.

**Symptoms**: The `L` command allowed cursor movement but pressing `t` didn't show the monster's description, and the game became unresponsive after spamming `f`.

**Cause**: Likely a bug in the `inspect_monster` function or the input handling for the `t` key in `main.cpp`. The `visible[target_y][target_x]` check in `inspect_monster` might be too restrictive, preventing monster selection. Spamming `f` could overwhelm the input buffer or cause rendering issues due to rapid state changes.

**Fix**: Modify `inspect_monster` to remove the `visible` check, ensuring it displays the monster's description if one exists at the target coordinates. Add a debounce mechanism for the `f` key to prevent input buffer issues.

3. **Spamming `f` Breaks the Game**:
   - **Symptoms**: Rapidly pressing `f` to toggle fog of war caused the game to become unresponsive.
   - **Cause**: Rapid state changes (`fog_enabled`) might overload the `ncurses` rendering in `draw_dungeon`, or the input buffer could be flooded, dropping subsequent inputs.
   - **Fix**: Implement a debounce mechanism for the `f` key to limit toggle frequency.

4. **Monster Parsing Error**:
   - **Symptoms**: The "Duplicate ability 'UNIQ' in ABIL, discarding monster" error persists, preventing SpongeBob SquarePants from spawning.
   - **Cause**: The `monster_parsing.cpp` fix to allow duplicate `UNIQ` abilities wasn’t applied correctly or was overwritten by a previous version in your repository.
   - **Fix**: Reapply the fix to ensure SpongeBob spawns, enabling forced combat testing.

5. **Tunneling and Combat**:
   - **Status**: The previous `dungeon_generation.cpp` fixed tunneling (monsters S and G leave empty spaces) and limited combat to one NPC per turn with `combat_triggered`. Forced combat for SpongeBob (5 damage per turn) is implemented but not triggering due to the parsing error.
   - **Action**: Preserve these fixes while addressing movement and interface issues.

### **Implementation Plan**
To restore direct player movement and fix the related issues:
1. **Restore Movement**:
   - Revert `ncurses` setup to a robust configuration using `raw()` mode, similar to your initial files, ensuring all movement keys (`7`, `8`, `k`, etc.) are captured.
   - Simplify the game loop in `main.cpp` to process movement keys directly, bypassing any mode-related blocks unless explicitly in `teleport_mode` or `inspect_mode`.
   - Verify `move_player` in `dungeon_generation.cpp` allows valid moves without restrictions.

2. **Fix `L` Command**:
   - Update `main.cpp` to ensure `t` in inspect mode triggers `inspect_monster` correctly.
   - Modify `inspect_monster` in `dungeon_generation.cpp` to remove the `visible` check, allowing monster descriptions to display when selected with `t`.

3. **Prevent `f` Spam Issues**:
   - Add a debounce mechanism for the `f` key in `main.cpp` to limit fog toggling frequency.
   - Ensure `draw_dungeon` is called consistently after state changes to maintain a stable display.

4. **Fix Monster Parsing**:
   - Reapply the `UNIQ` fix in `monster_parsing.cpp` to allow SpongeBob to spawn, enabling forced combat.

5. **Preserve Tunneling and Combat**:
   - Keep the tunneling fix (empty spaces for S and G) and single-combat-per-turn logic in `dungeon_generation.cpp`.
   - Maintain forced combat for SpongeBob (5 damage per turn, alternating).

6. **Remove Debug Output**:
   - Ensure all `fprintf` statements are removed for clean gameplay.

### **Modified Files**
Below are the updated files, designed to restore direct player movement, fix the `L` command, prevent `f` spam issues, and ensure SpongeBob spawns for forced combat. The changes are based on your latest provided versions, simplified to match the initial working movement behavior.

#### **1. Modified `monster_parsing.cpp`**
Reapply the fix to allow duplicate `UNIQ` abilities, ensuring SpongeBob SquarePants spawns.

<xaiArtifact artifact_id="576d34e0-2e37-4ee1-8990-77b213363bec" artifact_version_id="9840e9ab-16c1-4fb1-82eb-85b3930ad20f" title="monster_parsing.cpp" contentType="text/x-c++src">
#include "monster_parsing.h"
#include <fstream>
#include <iostream>
#include <sstream>
#include <cstring>
#include <stdlib.h>
#include <set>
#include <random>
#include "dungeon_generation.h"

MonsterDescription::MonsterDescription()
    : symbol('\0'), rarity(0), is_unique(false), is_alive(false) {}

void MonsterDescription::print() const {
    std::cout << name << "\n";
    for (const auto& line : description) {
        std::cout << line << "\n";
    }
    std::cout << symbol << "\n";
    for (size_t i = 0; i < colors.size(); ++i) {
        std::cout << colors[i];
        if (i < colors.size() - 1) std::cout << " ";
    }
    std::cout << "\n";
    std::cout << speed.toString() << "\n";
    for (size_t i = 0; i < abilities.size(); ++i) {
        std::cout << abilities[i];
        if (i < abilities.size() - 1) std::cout << " ";
    }
    std::cout << "\n";
    std::cout << hitpoints.toString() << "\n";
    std::cout << damage.toString() << "\n";
    std::cout << rarity << "\n";
    std::cout << (is_unique ? "UNIQUE" : "") << "\n";
    std::cout << "\n";
}

NPC* MonsterDescription::createNPC(int x, int y) {
    NPC* npc = new NPC(x, y);
    npc->name = name;
    npc->symbol = symbol;
    npc->color = colors.empty() ? "WHITE" : colors[0];
    npc->damage = damage;

    std::random_device rd;
    std::mt19937 gen(rd());
    npc->speed = speed.base;
    for (int i = 0; i < speed.dice; ++i) {
        std::uniform_int_distribution<> dis(1, speed.sides);
        npc->speed += dis(gen);
    }
    npc->hitpoints = hitpoints.base;
    for (int i = 0; i < hitpoints.dice; ++i) {
        std::uniform_int_distribution<> dis(1, hitpoints.sides);
        npc->hitpoints += dis(gen);
    }

    for (const auto& ability : abilities) {
        if (ability == "SMART") npc->intelligent = 1;
        else if (ability == "TELE") npc->telepathic = 1;
        else if (ability == "TUNNEL") npc->tunneling = 1;
        else if (ability == "ERRATIC") npc->erratic = 1;
        else if (ability == "PASS") npc->pass_wall = 1;
        else if (ability == "PICKUP") npc->pickup = 1;
        else if (ability == "DESTROY") npc->destroy = 1;
        else if (ability == "UNIQ") {
            is_unique = true;
            npc->is_unique = true;
        }
    }

    if (is_unique) {
        is_alive = true;
    }

    return npc;
}

std::vector<MonsterDescription> parseMonsterDescriptions(const std::string& filename) {
    std::vector<MonsterDescription> monsters;
    std::ifstream file(filename);
    if (!file.is_open()) {
        std::cerr << "Error: Cannot open " << filename << "\n";
        return monsters;
    }

    std::string line;
    std::getline(file, line);
    if (line != "RLG327 MONSTER DESCRIPTION 1") {
        std::cerr << "Error: Invalid file header in " << filename << "\n";
        file.close();
        exit(EXIT_FAILURE);
    }

    MonsterDescription current;
    bool inMonster = false;
    bool hasName = false, hasDesc = false, hasColor = false, hasSpeed = false;
    bool hasAbil = false, hasHP = false, hasDam = false, hasSymb = false, hasRarity = false;

    while (std::getline(file, line)) {
        if (line.empty()) continue;

        if (line == "BEGIN MONSTER") {
            if (inMonster) {
                std::cerr << "Nested BEGIN MONSTER detected, skipping previous monster\n";
            }
            current = MonsterDescription();
            inMonster = true;
            hasName = hasDesc = hasColor = hasSpeed = hasAbil = hasHP = hasDam = hasSymb = hasRarity = false;
            continue;
        }

        if (line == "END" && inMonster) {
            if (hasName && hasDesc && hasColor && hasSpeed && hasAbil && hasHP && hasDam && hasSymb && hasRarity) {
                monsters.push_back(current);
            } else {
                std::cerr << "Monster missing required fields, discarded\n";
            }
            inMonster = false;
            continue;
        }

        if (!inMonster) continue;

        std::istringstream iss(line);
        std::string keyword;
        iss >> keyword;

        if (keyword == "NAME") {
            if (hasName) {
                std::cerr << "Duplicate NAME, discarding monster\n";
                inMonster = false;
                continue;
            }
            std::getline(iss, current.name);
            current.name.erase(0, current.name.find_first_not_of(" \t"));
            hasName = true;
        } else if (keyword == "DESC") {
            if (hasDesc) {
                std::cerr << "Duplicate DESC, discarding monster\n";
                inMonster = false;
                continue;
            }
            hasDesc = true;
            current.description.clear();
            while (std::getline(file, line) && line != ".") {
                if (line.length() > 77) {
                    std::cerr << "Description line exceeds 77 characters, discarding monster\n";
                    inMonster = false;
                    break;
                }
                current.description.push_back(line);
            }
            if (!inMonster) continue;
        } else if (keyword == "COLOR") {
            if (hasColor) {
                std::cerr << "Duplicate COLOR, discarding monster\n";
                inMonster = false;
                continue;
            }
            std::string colorLine;
            std::getline(iss, colorLine);
            std::istringstream colorIss(colorLine);
            std::string color;
            while (colorIss >> color) {
                current.colors.push_back(color);
            }
            hasColor = true;
        } else if (keyword == "SPEED") {
            if (hasSpeed) {
                std::cerr << "Duplicate SPEED, discarding monster\n";
                inMonster = false;
                continue;
            }
            std::string diceStr;
            std::getline(iss, diceStr);
            if (sscanf(diceStr.c_str(), "%d+%dd%d", &current.speed.base, &current.speed.dice, &current.speed.sides) != 3) {
                std::cerr << "Invalid SPEED format, discarding monster\n";
                inMonster = false;
                continue;
            }
            hasSpeed = true;
        } else if (keyword == "ABIL") {
            if (hasAbil) {
                std::cerr << "Duplicate ABIL, discarding monster\n";
                inMonster = false;
                continue;
            }
            std::string abilLine;
            std::getline(iss, abilLine);
            std::istringstream abilIss(abilLine);
            std::string ability;
            std::set<std::string> seenAbilities;
            while (abilIss >> ability) {
                // Allow duplicate UNIQ to avoid discarding unique monsters
                if (ability != "UNIQ" && !seenAbilities.insert(ability).second) {
                    std::cerr << "Duplicate ability '" << ability << "' in ABIL, discarding monster\n";
                    inMonster = false;
                    break;
                }
                current.abilities.push_back(ability);
            }
            if (inMonster) hasAbil = true;
        } else if (keyword == "HP") {
            if (hasHP) {
                std::cerr << "Duplicate HP, discarding monster\n";
                inMonster = false;
                continue;
            }
            std::string diceStr;
            std::getline(iss, diceStr);
            if (sscanf(diceStr.c_str(), "%d+%dd%d", &current.hitpoints.base, &current.hitpoints.dice, &current.hitpoints.sides) != 3) {
                std::cerr << "Invalid HP format, discarding monster\n";
                inMonster = false;
                continue;
            }
            hasHP = true;
        } else if (keyword == "DAM") {
            if (hasDam) {
                std::cerr << "Duplicate DAM, discarding monster\n";
                inMonster = false;
                continue;
            }
            std::string diceStr;
            std::getline(iss, diceStr);
            if (sscanf(diceStr.c_str(), "%d+%dd%d", &current.damage.base, &current.damage.dice, &current.damage.sides) != 3) {
                std::cerr << "Invalid DAM format, discarding monster\n";
                inMonster = false;
                continue;
            }
            hasDam = true;
        } else if (keyword == "SYMB") {
            if (hasSymb) {
                std::cerr << "Duplicate SYMB, discarding monster\n";
                inMonster = false;
                continue;
            }
            std::string symb;
            iss >> symb;
            if (symb.length() == 1) {
                current.symbol = symb[0];
                hasSymb = true;
            } else {
                std::cerr << "SYMB must be a single character, discarding monster\n";
                inMonster = false;
            }
        } else if (keyword == "RRTY") {
            if (hasRarity) {
                std::cerr << "Duplicate RRTY, discarding monster\n";
                inMonster = false;
                continue;
            }
            iss >> current.rarity;
            if (current.rarity >= 1 && current.rarity <= 100) {
                hasRarity = true;
            } else {
                std::cerr << "RRTY must be between 1 and 100, discarding monster\n";
                inMonster = false;
            }
        }
    }

    file.close();
    return monsters;
}