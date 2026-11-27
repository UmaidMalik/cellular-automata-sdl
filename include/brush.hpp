#pragma once

#include "config.hpp"
#include <SDL3/SDL_rect.h>
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <numbers>
#include <span>
#include <vector>

enum class BrushMode
{
    Random,
    Solid,
    Erase
};

using BrushTexel = std::array<uint16_t, 4>;
// Match update_r1.frag: alpha stores a nine-bit state and seven-bit saturation.
constexpr uint16_t LIVE_CELL_STATE = 256;
constexpr int SATURATION_BITS = 7;

inline bool BrushCellIsAlive(Uint32 seed, int x, int y)
{
    Uint32 hash = seed ^ (static_cast<Uint32>(x) * 0x9e3779b9u) ^ (static_cast<Uint32>(y) * 0x85ebca6bu);
    hash ^= hash >> 16;
    hash *= 0x7feb352du;
    hash ^= hash >> 15;
    hash *= 0x846ca68bu;
    return ((hash ^ (hash >> 16)) >> 31) != 0;
}
static_assert(sizeof(BrushTexel) == 8);

struct BrushRow
{
    int left;
    int right;
    int y;

    int Width() const { return right - left; }
};

constexpr int MaxBrushSize(int grid_width)
{
    return std::max(1, grid_width / 5);
}

inline std::vector<BrushRow> RasterizeBrush(std::span<const SDL_Point> stroke, int size, int width, int height)
{
    if (stroke.empty())
        return {};

    const int half = size / 2;
    // Measure from cell centers in doubled coordinates so even diameters
    // can be centered between cells. Their outermost rows still get two cells.
    const int diameter = size - 1;
    const int radius_squared = diameter * diameter + (size % 2 == 0 ? 1 : 0);
    std::vector<int> insets(size);
    for (int y = 0; y < size; ++y)
    {
        const int dy = 2 * y - diameter;
        const int max_x = static_cast<int>(std::sqrt(radius_squared - dy * dy));
        insets[y] = (diameter - max_x + 1) / 2;
    }

    const auto bounds = std::ranges::minmax_element(stroke, {}, &SDL_Point::y);
    const int top = std::max(0, bounds.min->y - half);
    const int bottom = std::min(height, bounds.max->y - half + size);

    // The stroke centers form a contiguous line. Merge overlapping stamps
    // into one upload per row instead of issuing a copy for every cell.
    std::vector<BrushRow> rows(bottom - top);
    for (int y = top; y < bottom; ++y)
        rows[y - top] = {width, 0, y};

    for (const auto cell : stroke)
    {
        const int stamp_left = cell.x - half;
        const int stamp_top = cell.y - half;
        const int first_row = std::max(0, stamp_top);
        const int end_row = std::min(height, stamp_top + size);

        for (int y = first_row; y < end_row; ++y)
        {
            const int inset = insets[y - stamp_top];
            const int left = std::max(0, stamp_left + inset);
            const int right = std::min(width, stamp_left + size - inset);
            if (left < right)
            {
                BrushRow& row = rows[y - top];
                row.left = std::min(row.left, left);
                row.right = std::max(row.right, right);
            }
        }
    }

    return rows;
}

inline BrushTexel MakeBrushTexel(float hue, int saturation)
{
    // Store hue as a direction so the update pass can average colors across the hue seam.
    const float angle = 2.0f * std::numbers::pi_v<float> * hue;
    const auto saturation_percent = static_cast<uint16_t>(std::clamp(saturation,
        SimulationControls::MIN_BRUSH_SATURATION, SimulationControls::MAX_BRUSH_SATURATION));
    return BrushTexel{
        static_cast<uint16_t>(std::lround((std::cos(angle) * 0.5f + 0.5f) * UINT16_MAX)),
        static_cast<uint16_t>(std::lround((std::sin(angle) * 0.5f + 0.5f) * UINT16_MAX)),
        UINT16_MAX,
        static_cast<uint16_t>((LIVE_CELL_STATE << SATURATION_BITS) | saturation_percent)
    };
}

inline void FillBrushRow(BrushTexel* pixels, const BrushRow& row, Uint32 seed, BrushTexel alive, BrushMode mode)
{
    constexpr BrushTexel dead{};
    if (mode != BrushMode::Random)
    {
        std::fill_n(pixels, row.Width(), mode == BrushMode::Solid ? alive : dead);
        return;
    }
    for (int x = row.left; x < row.right; ++x)
        pixels[x - row.left] = BrushCellIsAlive(seed, x, row.y) ? alive : dead;
}
