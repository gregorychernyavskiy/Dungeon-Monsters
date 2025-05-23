#include "object_parsing.h"
#include <fstream>
#include <iostream>
#include <sstream>
#include <cstring>
#include <stdlib.h>
#include <set>
#include <random>
#include "dungeon_generation.h"

ObjectDescription::ObjectDescription()
    : name(), description(), types(), color(),
      hit(0, 0, 0), damage(0, 0, 0), dodge(0, 0, 0), defense(0, 0, 0),
      weight(0, 0, 0), speed(0, 0, 0), attribute(0, 0, 0), value(0, 0, 0),
      artifact(), rarity(0), range(0), is_artifact(false), is_created(false), heal(0, 0, 0) {}

void ObjectDescription::print() const {
    std::cout << name << "\n";
    for (const auto& line : description) {
        std::cout << line << "\n";
    }
    for (size_t i = 0; i < types.size(); ++i) {
        std::cout << types[i];
        if (i < types.size() - 1) std::cout << " ";
    }
    std::cout << "\n";
    std::cout << color << "\n";
    std::cout << hit.toString() << "\n";
    std::cout << damage.toString() << "\n";
    std::cout << dodge.toString() << "\n";
    std::cout << defense.toString() << "\n";
    std::cout << weight.toString() << "\n";
    std::cout << speed.toString() << "\n";
    std::cout << attribute.toString() << "\n";
    std::cout << value.toString() << "\n";
    std::cout << heal.toString() << "\n";
    std::cout << artifact << "\n";
    std::cout << rarity << "\n";
    std::cout << range << "\n";
    std::cout << "\n";
}

Object* ObjectDescription::createObject(int x, int y) {
    Object* obj = new Object(x, y);
    obj->name = name;
    obj->color = color;
    obj->damage = damage;
    obj->heal = heal;
    obj->types = types;
    obj->symbol = getObjectSymbol(types.empty() ? "" : types[0]);
    obj->range = range;

    std::random_device rd;
    std::mt19937 gen(rd());
    auto rollDice = [&](Dice d) -> int {
        int result = d.base;
        for (int i = 0; i < d.dice; ++i) {
            std::uniform_int_distribution<> dis(1, d.sides);
            result += dis(gen);
        }
        return result;
    };

    obj->hit = rollDice(hit);
    obj->dodge = rollDice(dodge);
    obj->defense = rollDice(defense);
    obj->weight = rollDice(weight);
    obj->speed = rollDice(speed);
    obj->attribute = rollDice(attribute);
    obj->value = rollDice(value);

    if (artifact == "TRUE") {
        is_artifact = true;
        obj->is_artifact = true;
        is_created = true;
    }

    return obj;
}

char getObjectSymbol(const std::string& type) {
    if (type == "WEAPON") return '|';
    if (type == "OFFHAND") return ')';
    if (type == "RANGED") return '}';
    if (type == "ARMOR") return '[';
    if (type == "HELMET") return ']';
    if (type == "CLOAK") return '(';
    if (type == "GLOVES") return '{';
    if (type == "BOOTS") return '{';
    if (type == "RING") return '=';
    if (type == "AMULET") return '"';
    if (type == "LIGHT") return '_';
    if (type == "SCROLL") return '~';
    if (type == "BOOK") return '?';
    if (type == "FLASK") return '!';
    if (type == "GOLD") return '$';
    if (type == "AMMUNITION") return '/';
    if (type == "FOOD") return ',';
    if (type == "WAND") return '-';
    if (type == "CONTAINER") return '%';
    if (type == "SPELLBOOK") return '?';
    return '*';
}

std::vector<ObjectDescription> parseObjectDescriptions(const std::string& filename) {
    std::vector<ObjectDescription> objects;
    std::ifstream file(filename);
    if (!file.is_open()) {
        std::cerr << "Error: Cannot open " << filename << "\n";
        return objects;
    }

    std::string line;
    std::getline(file, line);
    if (line != "RLG327 OBJECT DESCRIPTION 1") {
        std::cerr << "Error: Invalid file header in " << filename << "\n";
        file.close();
        exit(EXIT_FAILURE);
    }

    ObjectDescription current;
    bool inObject = false;
    bool hasName = false, hasDesc = false, hasType = false, hasColor = false;
    bool hasHit = false, hasDam = false, hasDodge = false, hasDef = false;
    bool hasWeight = false, hasSpeed = false, hasAttr = false, hasVal = false;
    bool hasArt = false, hasRarity = false, hasHeal = false;

    while (std::getline(file, line)) {
        if (line.empty()) continue;

        if (line == "BEGIN OBJECT") {
            if (inObject) {
                std::cerr << "Nested BEGIN OBJECT detected, skipping previous object\n";
            }
            current = ObjectDescription();
            inObject = true;
            hasName = hasDesc = hasType = hasColor = hasHit = hasDam = hasDodge = hasDef = false;
            hasWeight = hasSpeed = hasAttr = hasVal = hasArt = hasRarity = hasHeal = false;
            continue;
        }

        if (line == "END" && inObject) {
            if (hasName && hasDesc && hasType && hasColor && hasHit && hasDam && hasDodge && hasDef &&
                hasWeight && hasSpeed && hasAttr && hasVal && hasArt && hasRarity) {
                objects.push_back(current);
            } else {
                std::cerr << "Object missing required fields, discarded\n";
            }
            inObject = false;
            continue;
        }

        if (!inObject) continue;

        std::istringstream iss(line);
        std::string keyword;
        iss >> keyword;

        if (keyword == "NAME") {
            if (hasName) {
                std::cerr << "Duplicate NAME, discarding object\n";
                inObject = false;
                continue;
            }
            std::getline(iss, current.name);
            current.name.erase(0, current.name.find_first_not_of(" \t"));
            hasName = true;
        } else if (keyword == "DESC") {
            if (hasDesc) {
                std::cerr << "Duplicate DESC, discarding object\n";
                inObject = false;
                continue;
            }
            hasDesc = true;
            current.description.clear();
            while (std::getline(file, line) && line != ".") {
                if (line.length() > 77) {
                    std::cerr << "Description line exceeds 77 characters, discarding object\n";
                    inObject = false;
                    break;
                }
                current.description.push_back(line);
            }
            if (!inObject) continue;
        } else if (keyword == "TYPE") {
            if (hasType) {
                std::cerr << "Duplicate TYPE, discarding object\n";
                inObject = false;
                continue;
            }
            std::string typeLine;
            std::getline(iss, typeLine);
            std::istringstream typeIss(typeLine);
            std::string type;
            std::set<std::string> seenTypes;
            while (typeIss >> type) {
                if (!seenTypes.insert(type).second) {
                    std::cerr << "Duplicate type '" << type << "' in TYPE, discarding object\n";
                    inObject = false;
                    break;
                }
                current.types.push_back(type);
            }
            if (inObject) hasType = true;
        } else if (keyword == "COLOR") {
            if (hasColor) {
                std::cerr << "Duplicate COLOR, discarding object\n";
                inObject = false;
                continue;
            }
            iss >> current.color;
            hasColor = true;
        } else if (keyword == "HIT") {
            if (hasHit) {
                std::cerr << "Duplicate HIT, discarding object\n";
                inObject = false;
                continue;
            }
            std::string diceStr;
            std::getline(iss, diceStr);
            if (sscanf(diceStr.c_str(), "%d+%dd%d", &current.hit.base, &current.hit.dice, &current.hit.sides) != 3) {
                std::cerr << "Invalid HIT format, discarding object\n";
                inObject = false;
                continue;
            }
            hasHit = true;
        } else if (keyword == "DAM") {
            if (hasDam) {
                std::cerr << "Duplicate DAM, discarding object\n";
                inObject = false;
                continue;
            }
            std::string diceStr;
            std::getline(iss, diceStr);
            if (sscanf(diceStr.c_str(), "%d+%dd%d", &current.damage.base, &current.damage.dice, &current.damage.sides) != 3) {
                std::cerr << "Invalid DAM format, discarding object\n";
                inObject = false;
                continue;
            }
            hasDam = true;
        } else if (keyword == "DODGE") {
            if (hasDodge) {
                std::cerr << "Duplicate DODGE, discarding object\n";
                inObject = false;
                continue;
            }
            std::string diceStr;
            std::getline(iss, diceStr);
            if (sscanf(diceStr.c_str(), "%d+%dd%d", &current.dodge.base, &current.dodge.dice, &current.dodge.sides) != 3) {
                std::cerr << "Invalid DODGE format, discarding object\n";
                inObject = false;
                continue;
            }
            hasDodge = true;
        } else if (keyword == "DEF") {
            if (hasDef) {
                std::cerr << "Duplicate DEF, discarding object\n";
                inObject = false;
                continue;
            }
            std::string diceStr;
            std::getline(iss, diceStr);
            if (sscanf(diceStr.c_str(), "%d+%dd%d", &current.defense.base, &current.defense.dice, &current.defense.sides) != 3) {
                std::cerr << "Invalid DEF format, discarding object\n";
                inObject = false;
                continue;
            }
            hasDef = true;
        } else if (keyword == "WEIGHT") {
            if (hasWeight) {
                std::cerr << "Duplicate WEIGHT, discarding object\n";
                inObject = false;
                continue;
            }
            std::string diceStr;
            std::getline(iss, diceStr);
            if (sscanf(diceStr.c_str(), "%d+%dd%d", &current.weight.base, &current.weight.dice, &current.weight.sides) != 3) {
                std::cerr << "Invalid WEIGHT format, discarding object\n";
                inObject = false;
                continue;
            }
            hasWeight = true;
        } else if (keyword == "SPEED") {
            if (hasSpeed) {
                std::cerr << "Duplicate SPEED, discarding object\n";
                inObject = false;
                continue;
            }
            std::string diceStr;
            std::getline(iss, diceStr);
            if (sscanf(diceStr.c_str(), "%d+%dd%d", &current.speed.base, &current.speed.dice, &current.speed.sides) != 3) {
                std::cerr << "Invalid SPEED format, discarding object\n";
                inObject = false;
                continue;
            }
            hasSpeed = true;
        } else if (keyword == "ATTR") {
            if (hasAttr) {
                std::cerr << "Duplicate ATTR, discarding object\n";
                inObject = false;
                continue;
            }
            std::string diceStr;
            std::getline(iss, diceStr);
            if (sscanf(diceStr.c_str(), "%d+%dd%d", &current.attribute.base, &current.attribute.dice, &current.attribute.sides) != 3) {
                std::cerr << "Invalid ATTR format, discarding object\n";
                inObject = false;
                continue;
            }
            hasAttr = true;
        } else if (keyword == "VAL") {
            if (hasVal) {
                std::cerr << "Duplicate VAL, discarding object\n";
                inObject = false;
                continue;
            }
            std::string diceStr;
            std::getline(iss, diceStr);
            if (sscanf(diceStr.c_str(), "%d+%dd%d", &current.value.base, &current.value.dice, &current.value.sides) != 3) {
                std::cerr << "Invalid VAL format, discarding object\n";
                inObject = false;
                continue;
            }
            hasVal = true;
        } else if (keyword == "HEAL") {
            if (hasHeal) {
                std::cerr << "Duplicate HEAL, discarding object\n";
                inObject = false;
                continue;
            }
            std::string diceStr;
            std::getline(iss, diceStr);
            if (sscanf(diceStr.c_str(), "%d+%dd%d", &current.heal.base, &current.heal.dice, &current.heal.sides) != 3) {
                std::cerr << "Invalid HEAL format, discarding object\n";
                inObject = false;
                continue;
            }
            hasHeal = true;
        } else if (keyword == "ART") {
            if (hasArt) {
                std::cerr << "Duplicate ART, discarding object\n";
                inObject = false;
                continue;
            }
            iss >> current.artifact;
            if (current.artifact != "TRUE" && current.artifact != "FALSE") {
                std::cerr << "ART must be TRUE or FALSE, discarding object\n";
                inObject = false;
                continue;
            }
            hasArt = true;
        } else if (keyword == "RRTY" || keyword == "RARITY") {
            if (hasRarity) {
                std::cerr << "Duplicate RARITY, discarding object\n";
                inObject = false;
                continue;
            }
            iss >> current.rarity;
            if (current.rarity >= 1 && current.rarity <= 100) {
                hasRarity = true;
            } else {
                std::cerr << "RARITY must be between 1 and 100, discarding object\n";
                inObject = false;
            }
        } else if (keyword == "RANGE") {
            iss >> current.range;
            if (current.range < 0) {
                std::cerr << "RANGE must be non-negative, discarding object\n";
                inObject = false;
                continue;
            }
        }
    }

    file.close();
    return objects;
}