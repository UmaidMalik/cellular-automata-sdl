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
    {"Brian's Brain", RuleMask({2}), {}, 1, 1},
    {"Star Wars", RuleMask({2}), RuleMask({3, 4, 5}), 1, 2},
    // Golly includes the center in Bosco's survival count. This app excludes it.
    {"Bosco's Rule", RuleRange(34, 45), RuleRange(33, 57), 5, 0},
};

constexpr int NeighborhoodWidth(int radius)
{
    return radius * 2 + 1;
}

constexpr int NeighborhoodMaxCount(int radius)
{
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
    const int max_count = NeighborhoodMaxCount(controls.neighbor_radius);
    for (const auto& preset : RULE_PRESETS)
    {
        if (preset.radius != controls.neighbor_radius || preset.aging_stages != aging)
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
    controls.aging_enabled = preset.aging_stages > 0;
    if (controls.aging_enabled)
        controls.aging_stages = preset.aging_stages;
}
