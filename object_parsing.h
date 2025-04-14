#ifndef OBJECT_PARSING_H
#define OBJECT_PARSING_H

#include <string>
#include <vector>
#include <stdint.h>
#include "dice.h"

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