#include "object_parsing.h"
#include <fstream>
#include <iostream>
#include <sstream>
#include <cstring>
#include <stdlib.h>
#include <set>

ObjectDescription::ObjectDescription()
    : rarity(0) {}

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
    std::cout << artifact << "\n";
    std::cout << rarity << "\n";
    std::cout << "\n";
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
    bool hasArt = false, hasRarity = false;

    while (std::getline(file, line)) {
        if (line.empty()) continue;

        if (line == "BEGIN OBJECT") {
            if (inObject) {
                std::cerr << "Nested BEGIN OBJECT detected, skipping previous object\n";
            }
            current = ObjectDescription();
            inObject = true;
            hasName = hasDesc = hasType = hasColor = hasHit = hasDam = hasDodge = hasDef = false;
            hasWeight = hasSpeed = hasAttr = hasVal = hasArt = hasRarity = false;
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
            std::set<std::string> seenTypes; // Check for duplicate types
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
            iss >> current.color; // Single color for simplicity
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
        } else if (keyword == "DODGE") { // Corrected from DOOGE to DODGE
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
        } else if (keyword == "RRTY") {
            if (hasRarity) {
                std::cerr << "Duplicate RRTY, discarding object\n";
                inObject = false;
                continue;
            }
            iss >> current.rarity;
            if (current.rarity >= 1 && current.rarity <= 100) {
                hasRarity = true;
            } else {
                std::cerr << "RRTY must be between 1 and 100, discarding object\n";
                inObject = false;
            }
        }
    }

    file.close();
    return objects;
}