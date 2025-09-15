#pragma once

#include <iostream>
#include <cstdint>

struct Cell
{
    uint8_t state = 0; // 0: DEAD, 1: ALIVE, 2: AGING1 ... 
    uint8_t color_r = 0; // 0-255
    uint8_t color_g = 0; // 0-255
    uint8_t color_b = 0; // 0-255
};


std::ostream& operator<<(std::ostream& os, const Cell& cell)
{
    os  << "{"
        << static_cast<int>(cell.color_r) << ","
        << static_cast<int>(cell.color_g) << ","
        << static_cast<int>(cell.color_b) << "}["
        << static_cast<int>(cell.state)     << "]";
    return os;
}