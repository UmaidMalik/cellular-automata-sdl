#pragma once

struct config
{
    static constexpr int WINDOW_WIDTH = 1280;
    static constexpr int WINDOW_HEIGHT = 720;
    static constexpr const char* WINDOW_TITLE = "Cellular Automata";
    static constexpr int GRID_WIDTH = 3200;
    static constexpr int GRID_HEIGHT = 1800;
    static constexpr int MAX_STATES = 8;
};

struct SimulationControls
{
    static constexpr int NEIGHBOR_RANGE = 1;
    static constexpr int MAX_NEIGHBORS = 8;

    bool paused = false;
    bool wrap_edges = true;
    float color_decay = 0.999f;
    unsigned int birth_mask = (1u << 2) | (1u << 3);
    unsigned int survival_mask = 1u << 2;
};
