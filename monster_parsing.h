#ifndef MONSTER_PARSING_H
#define MONSTER_PARSING_H

#include <string>
#include <vector>
#include <stdint.h>
#include "dice.h"

// Forward declaration of NPC
class NPC;

class MonsterDescription {
public:
    std::string name;
    std::vector<std::string> description;
    std::vector<std::string> colors;
    Dice speed;
    std::vector<std::string> abilities;
    Dice hitpoints;
    Dice damage;
    char symbol;
    int rarity;
    bool is_unique; // Track if monster is unique
    bool is_alive;  // Track if unique monster is alive

    MonsterDescription();
    void print() const;
    NPC* createNPC(int x, int y); // Factory method to create NPC instance
};

std::vector<MonsterDescription> parseMonsterDescriptions(const std::string& filename);

#endif