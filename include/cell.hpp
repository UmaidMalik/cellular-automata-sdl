#pragma once

#include <iostream>
#include <cstdint>
#include "math.hpp"

using namespace math;

struct Cell
{
    uint8_t state = 0; // 0: DEAD, 1: ALIVE, 2: AGING1 ... 
    HSVColor color{0.0f, 0.0f, 0.0f};
};


std::ostream& operator<<(std::ostream& os, const Cell& cell)
{
    os  << "{"
        << static_cast<int>(cell.color.h) << ","
        << static_cast<int>(cell.color.s) << ","
        << static_cast<int>(cell.color.v) << "}["
        << static_cast<int>(cell.state)     << "]";
    return os;
}