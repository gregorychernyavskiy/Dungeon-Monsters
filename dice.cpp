// dice.cpp
#include "dice.h"

std::string Dice::toString() const {
    return std::to_string(base) + "+" + std::to_string(dice) + "d" + std::to_string(sides);
}