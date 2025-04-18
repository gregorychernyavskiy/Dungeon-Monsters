#ifndef OBJECT_PARSING_H
#define OBJECT_PARSING_H

#include <string>
#include <vector>
#include <stdint.h>
#include "dice.h"

// Forward declaration of Object
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
    std::string artifact; // "TRUE" or "FALSE"
    int rarity;
    bool is_artifact;    // Track if object is an artifact
    bool is_created;     // Track if artifact has been created

    ObjectDescription();
    void print() const;
    Object* createObject(int x, int y); // Factory method
};

std::vector<ObjectDescription> parseObjectDescriptions(const std::string& filename);
char getObjectSymbol(const std::string& type); // Declare here

#endif