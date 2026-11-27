#pragma once

#include "config.hpp"
#include <initializer_list>

struct RulePreset
{
    const char* name;
    NeighborRule birth_mask;
    NeighborRule survival_mask;
    int radius;
    int aging_stages;
    NeighborhoodShape shape = NeighborhoodShape::Moore;
};

constexpr NeighborRule RuleMask(std::initializer_list<unsigned int> counts)
{
    NeighborRule mask{};
    for (const auto count : counts)
        mask[count / 32] |= 1u << (count % 32);
    return mask;
}

constexpr NeighborRule RuleRange(unsigned int first, unsigned int last)
{
    NeighborRule mask{};
    for (unsigned int count = first; count <= last; ++count)
        mask[count / 32] |= 1u << (count % 32);
    return mask;
}

inline constexpr RulePreset RULE_PRESETS[]{
    {"Conway's Game of Life", RuleMask({3}), RuleMask({2, 3}), 1, 0},
    {"HighLife", RuleMask({3, 6}), RuleMask({2, 3}), 1, 0},
    {"Seeds", RuleMask({2}), {}, 1, 0},
    {"Day & Night", RuleMask({3, 6, 7, 8}), RuleMask({3, 4, 6, 7, 8}), 1, 0},
    {"Diamoeba", RuleMask({3, 5, 6, 7, 8}), RuleMask({5, 6, 7, 8}), 1, 0},
    {"LongLife", RuleMask({3, 4, 5}), RuleMask({5}), 1, 0},
    {"Persian Rug", RuleMask({2, 3, 4}), {}, 1, 0},
    {"2x2", RuleMask({3, 6}), RuleMask({1, 2, 5}), 1, 0},
    {"DotLife", RuleMask({3}), RuleMask({0, 2, 3}), 1, 0},
    {"DryLife", RuleMask({3, 7}), RuleMask({2, 3}), 1, 0},
    {"Flock", RuleMask({3}), RuleMask({1, 2}), 1, 0},
    {"Gems", RuleMask({3, 4, 5, 7}), RuleMask({4, 5, 6, 8}), 1, 0},
    {"HoneyLife", RuleMask({3, 8}), RuleMask({2, 3, 8}), 1, 0},
    {"Life Without Death", RuleMask({3}), RuleRange(0, 8), 1, 0},
    {"Live Free or Die", RuleMask({2}), RuleMask({0}), 1, 0},
    {"Logarithmic Replicator", RuleMask({3, 6}), RuleMask({2, 4, 5}), 1, 0},
    {"LowDeath", RuleMask({3, 6, 8}), RuleMask({2, 3, 8}), 1, 0},
    {"Maze", RuleMask({3}), RuleRange(1, 5), 1, 0},
    {"Morley (Move)", RuleMask({3, 6, 8}), RuleMask({2, 4, 5}), 1, 0},
    {"Replicator", RuleMask({1, 3, 5, 7}), RuleMask({1, 3, 5, 7}), 1, 0},
    {"Stains", RuleMask({3, 6, 7, 8}), RuleMask({2, 3, 5, 6, 7, 8}), 1, 0},
    // Published Generations state totals include both the live and dead states.
    {"Brian's Brain", RuleMask({2}), {}, 1, 1},
    {"Banners", RuleMask({3, 4, 5, 7}), RuleMask({2, 3, 6, 7}), 1, 3},
    {"Bloomerang", RuleMask({3, 4, 6, 7, 8}), RuleMask({2, 3, 4}), 1, 22},
    {"Caterpillars", RuleMask({3, 7, 8}), RuleMask({1, 2, 4, 5, 6, 7}), 1, 2},
    {"Cooties", RuleMask({2}), RuleMask({2, 3}), 1, 6},
    {"Fireworks", RuleMask({1, 3}), RuleMask({2}), 1, 19},
    {"Frogs", RuleMask({3, 4}), RuleMask({1, 2}), 1, 1},
    {"Lava", RuleMask({4, 5, 6, 7, 8}), RuleMask({1, 2, 3, 4, 5}), 1, 6},
    {"Lines", RuleMask({4, 5, 8}), RuleMask({0, 1, 2, 3, 4, 5}), 1, 1},
    {"Star Wars", RuleMask({2}), RuleMask({3, 4, 5}), 1, 2},
    {"Sticks", RuleMask({2}), RuleMask({3, 4, 5, 6}), 1, 4},
    {"Transers", RuleMask({2, 6}), RuleMask({3, 4, 5}), 1, 3},
    {"Xtasy", RuleMask({2, 3, 5, 6}), RuleMask({1, 4, 5, 6}), 1, 14},
    // Convert center-inclusive survival ranges. Majority's birth range tops out at 80 neighbors.
    {"Majority", RuleRange(41, 80), RuleRange(40, 80), 4, 0},
    {"Bosco's Rule", RuleRange(34, 45), RuleRange(33, 57), 5, 0},
    {"Gnarl", RuleMask({1}), RuleMask({0}), 1, 0, NeighborhoodShape::VonNeumann},
    {"B13/S012V", RuleMask({1, 3}), RuleMask({0, 1, 2}), 1, 0, NeighborhoodShape::VonNeumann},
    {"B2/S013V", RuleMask({2}), RuleMask({0, 1, 3}), 1, 0, NeighborhoodShape::VonNeumann},
    {"Circular Growth", RuleRange(8, 10), RuleRange(7, 12), 3, 0, NeighborhoodShape::Circular},
    {"B245/S3H", RuleMask({2, 4, 5}), RuleMask({3}), 1, 0, NeighborhoodShape::Hexagonal},
};

constexpr int NeighborhoodWidth(int radius)
{
    return radius * 2 + 1;
}

constexpr int NeighborhoodMaxCount(int radius, NeighborhoodShape shape = NeighborhoodShape::Moore)
{
    if (shape == NeighborhoodShape::VonNeumann)
        return 2 * radius * (radius + 1);
    if (shape == NeighborhoodShape::Hexagonal)
        return 3 * radius * (radius + 1);
    if (shape == NeighborhoodShape::Circular)
    {
        int count = 0;
        for (int y = -radius; y <= radius; ++y)
            for (int x = -radius; x <= radius; ++x)
                count += x * x + y * y <= radius * radius;
        return count - 1;
    }
    const int width = NeighborhoodWidth(radius);
    return width * width - 1;
}

constexpr bool RuleHasCount(const NeighborRule& mask, int count)
{
    return (mask[count / 32] & (1u << (count % 32))) != 0;
}

inline const RulePreset* FindActiveRulePreset(const SimulationControls& controls)
{
    const int aging = controls.aging_enabled ? controls.aging_stages : 0;
    const int max_count = NeighborhoodMaxCount(controls.neighbor_radius, controls.neighborhood_shape);
    for (const auto& preset : RULE_PRESETS)
    {
        if (preset.shape != controls.neighborhood_shape || preset.radius != controls.neighbor_radius || preset.aging_stages != aging)
            continue;

        int count = 0;
        for (; count <= max_count; ++count)
        {
            if (RuleHasCount(preset.birth_mask, count) != RuleHasCount(controls.birth_mask, count) ||
                RuleHasCount(preset.survival_mask, count) != RuleHasCount(controls.survival_mask, count))
                break;
        }
        if (count > max_count)
            return &preset;
    }
    return nullptr;
}

inline void ApplyRulePreset(SimulationControls& controls, const RulePreset& preset)
{
    controls.birth_mask = preset.birth_mask;
    controls.survival_mask = preset.survival_mask;
    controls.neighbor_radius = preset.radius;
    controls.neighborhood_shape = preset.shape;
    controls.aging_enabled = preset.aging_stages > 0;
    if (controls.aging_enabled)
        controls.aging_stages = preset.aging_stages;
}
