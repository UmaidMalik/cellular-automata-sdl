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
    static constexpr int MIN_GRID_SIZE = 128;
    static constexpr int MAX_GRID_SIZE = 3200;
    static constexpr int MIN_BRUSH_SATURATION = 25;
    static constexpr int MAX_BRUSH_SATURATION = 100;

    int neighbor_radius = 1;
    bool aging_enabled = false;
    int aging_stages = 6;
    int grid_size = config::GRID_WIDTH;
    int brush_size = 12;
    bool erase_brush = false;
    bool solid_brush = false;
    bool sync_brush_with_zoom = true;
    int generations_per_second = 60;
    bool paused = false;
    bool pause_on_draw = false;
    bool wrap_edges = true;
    bool random_brush_hue = true;
    float brush_hue = 0.5f;
    int brush_saturation = MAX_BRUSH_SATURATION;
    float color_decay = 0.999f;
    NeighborRule birth_mask{1u << 3};
    NeighborRule survival_mask{(1u << 2) | (1u << 3)};
};
