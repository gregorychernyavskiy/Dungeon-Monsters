#include "dice.h"
#include <random>

std::string Dice::toString() const {
    return std::to_string(base) + "+" + std::to_string(dice) + "d" + std::to_string(sides);
}

int Dice::roll() const {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(1, sides);
    int result = base;
    for (int i = 0; i < dice; ++i) {
        result += dis(gen);
    }
    return result;
}