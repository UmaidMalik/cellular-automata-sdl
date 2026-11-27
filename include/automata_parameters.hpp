#pragma once

#include "config.hpp"
#include <cstdint>

struct alignas(16) AutomataParams
{
    int32_t width, height, range, wrap;
    int32_t aging_stages;
    float decay;
    int32_t padding[2];
    NeighborRule birth_mask;
    NeighborRule survival_mask;
};
static_assert(sizeof(AutomataParams) == 64);

inline AutomataParams MakeAutomataParams(int width, int height, const SimulationControls& controls)
{
    return AutomataParams{
        width, height, controls.neighbor_radius, controls.wrap_edges,
        controls.aging_enabled ? controls.aging_stages : 0, controls.color_decay, {}, controls.birth_mask, controls.survival_mask
    };
}
