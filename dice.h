#include "dice.h"
#include <cstdlib>

std::string Dice::toString() const {
    return std::to_string(base) + "+" + std::to_string(dice) + "d" + std::to_string(sides);
}

int Dice::roll() const {
    int result = base;
    for (int i = 0; i < dice; i++) {
        result += (rand() % sides) + 1;
    }
    return result;
}