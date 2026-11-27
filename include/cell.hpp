#pragma once

#include <iostream>
#include <fmt/core.h>
#include <fmt/ostream.h>
#include <cstdint>
#include "math.hpp"

using namespace math;

struct Cell
{
    int state = 0; // 0: DEAD, 1: ALIVE, 2: AGING1 ... 
    HSVColor color{0.0f, 0.0f, 0.0f};
    Cell() = default;
    Cell(int s, HSVColor c) : state(s), color(c) {};
};

enum class CellState
{
    DEAD = 0,
    ALIVE = 1,
    AGING = 2, // 2 to n states
};


std::ostream& operator<<(std::ostream& os, const Cell& cell)
{
    os  << "{"
        << static_cast<float>(cell.color.h) << ","
        << static_cast<float>(cell.color.s) << ","
        << static_cast<float>(cell.color.v) << "}["
        << static_cast<int>(cell.state)     << "]";
    return os;
}

template <>
struct fmt::formatter<Cell>
{
    constexpr auto parse(format_parse_context& ctx)
    {
        return ctx.begin();
    }

    template <typename FormatContext>
    auto format(const Cell& cell, FormatContext& ctx) const
    {
        return fmt::format_to
        (
            ctx.out(), "{{{:.2f},{:.2f},{:.2f}}}[{}]",
            static_cast<float>(cell.color.h),
            static_cast<float>(cell.color.s),
            static_cast<float>(cell.color.v),
            static_cast<int>(cell.state)
        );
    }
};