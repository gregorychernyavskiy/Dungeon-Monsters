#ifndef OBJECT_PARSING_H
#define OBJECT_PARSING_H

#include <string>
#include <vector>
#include "dice.h"

class Object;

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
    std::string artifact;
    int rarity;        // Moved up
    int range;         // Moved up
    bool is_artifact;  // Moved up
    bool is_created;   // Moved up
    Dice heal;         // Moved to end of this group

    ObjectDescription();
    void print() const;
    Object* createObject(int x, int y);
};

std::vector<ObjectDescription> parseObjectDescriptions(const std::string& filename);
char getObjectSymbol(const std::string& type);

#endif