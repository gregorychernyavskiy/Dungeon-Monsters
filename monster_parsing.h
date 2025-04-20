#ifndef MONSTER_PARSING_H
#define MONSTER_PARSING_H

#include <string>
#include <vector>
#include <stdint.h>
#include "dice.h"

class NPC;

class MonsterDescription {
public:
    std::string name;
    std::vector<std::string> description;
    std::vector<std::string> colors;
    Dice speed;
    std::vector<std::string> abilities;
    char symbol;
    int rarity;
    bool is_unique;
    bool is_alive;

    MonsterDescription();
    void print() const;
    NPC* createNPC(int x, int y);
};

std::vector<MonsterDescription> parseMonsterDescriptions(const std::string& filename);

#endif