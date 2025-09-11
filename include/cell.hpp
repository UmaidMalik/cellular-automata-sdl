#pragma once


enum State
{
    DEAD,
    ALIVE,
    AGING1,
    AGING2,
    AGING3
};

struct Cell
{
    State state;
    SDL_Color color;
};
