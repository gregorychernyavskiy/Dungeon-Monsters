#ifndef MONSTER_PARSING_H
#define MONSTER_PARSING_H

#include <string>
#include <vector>
#include <stdint.h>

struct Dice {
    int base;
    int dice;
    int sides;
    Dice() : base(0), dice(0), sides(0) {}
    Dice(int b, int d, int s) : base(b), dice(d), sides(s) {}
    std::string toString() const;
};

class MonsterDescription {
public:
    std::string name;
    std::vector<std::string> description; // Multi-line description
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