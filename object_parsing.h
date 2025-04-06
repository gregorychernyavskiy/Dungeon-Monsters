#ifndef OBJECT_PARSING_H
#define OBJECT_PARSING_H

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

class ObjectDescription {
public:
    std::string name;
    std::vector<std::string> description;
    std::vector<std::string> types;      
    std::string color;                    
    Dice hit;
    Dice damage;
    Dice dodge;
    Dice defense;
    Dice weight;
    Dice speed;
    Dice attribute;
    Dice value;
    std::string artifact; // "TRUE" or "FALSE"
    int rarity;

    ObjectDescription();
    void print() const;
};

std::vector<ObjectDescription> parseObjectDescriptions(const std::string& filename);

#endif