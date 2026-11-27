#pragma once

#include <array>

using NeighborRule = std::array<unsigned int, 4>;

struct config
{
    static constexpr int WINDOW_WIDTH = 1280;
    static constexpr int WINDOW_HEIGHT = 720;
    static constexpr const char* WINDOW_TITLE = "Cellular Automata";
    static constexpr int GRID_WIDTH = 3200;
    static constexpr int GRID_HEIGHT = 1800;
};

struct SimulationControls
{
    static constexpr int MAX_NEIGHBOR_RADIUS = 5;

    static constexpr int MIN_GENERATIONS_PER_SECOND = 1;
    static constexpr int MAX_GENERATIONS_PER_SECOND = 120;

    static constexpr int MAX_AGING_STAGES = 255;
    static constexpr int MAX_BRUSH_SIZE = 500;

    int neighbor_radius = 1;
    bool aging_enabled = false;
    int aging_stages = 6;
    int brush_size = 1;
    bool sync_brush_with_zoom = false;
    int generations_per_second = 60;
    bool paused = true;
    bool pause_on_draw = false;
    bool wrap_edges = true;
    bool random_brush_hue = true;
    float brush_hue = 0.5f;
    int color_saturation = 100;
    float color_decay = 0.999f;
    NeighborRule birth_mask{1u << 3};
    NeighborRule survival_mask{(1u << 2) | (1u << 3)};
};
