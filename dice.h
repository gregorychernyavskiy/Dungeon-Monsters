// dice.h
#ifndef DICE_H
#define DICE_H

#include <string>

struct Dice {
    int base;
    int dice;
    int sides;
    Dice() : base(0), dice(0), sides(0) {}
    Dice(int b, int d, int s) : base(b), dice(d), sides(s) {}
    std::string toString() const;
};

#endif