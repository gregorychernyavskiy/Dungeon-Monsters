#ifndef MONSTER_PARSING_H
#define MONSTER_PARSING_H

#include <string>
#include <vector>
#include <stdint.h>
#include "dice.h"

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

    MonsterDescription();
    void print() const;
};

std::vector<MonsterDescription> parseMonsterDescriptions(const std::string& filename);

#endif