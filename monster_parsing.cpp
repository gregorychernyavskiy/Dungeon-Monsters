#include "monster_parsing.h"
#include <fstream>
#include <iostream>
#include <sstream>
#include <cstring>
#include <stdlib.h>
#include <set>

MonsterDescription::MonsterDescription()
    : symbol('\0'), rarity(0) {}

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
    std::cout << "\n";
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
            std::set<std::string> seenAbilities; // Track unique abilities
            while (abilIss >> ability) {
                if (!seenAbilities.insert(ability).second) { // Check for duplicates
                    std::cerr << "Duplicate ability '" << ability << "' in ABIL, discarding monster\n";
                    inMonster = false;
                    break;
                }
                current.abilities.push_back(ability);
            }
            if (inMonster) hasAbil = true; // Only set if no duplicates found
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