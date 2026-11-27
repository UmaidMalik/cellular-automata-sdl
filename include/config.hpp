#pragma once

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
    static constexpr int NEIGHBOR_RANGE = 1;
    static constexpr int MAX_NEIGHBORS = 8;

    static constexpr int MIN_GENERATIONS_PER_SECOND = 1;
    static constexpr int MAX_GENERATIONS_PER_SECOND = 120;

    static constexpr int MAX_AGING_STAGES = 255;
    static constexpr int MAX_BRUSH_SIZE = 64;

    bool aging_enabled = false;
    int aging_stages = 6;
    int brush_size = 1;
    int generations_per_second = 60;
    bool paused = true;
    bool wrap_edges = true;
    bool random_brush_hue = true;
    float brush_hue = 0.5f;
    float color_decay = 0.999f;
    unsigned int birth_mask = 1u << 3;
    unsigned int survival_mask = (1u << 2) | (1u << 3);
};
