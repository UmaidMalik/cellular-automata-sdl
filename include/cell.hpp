#pragma once

namespace ca
{
    enum state
    {
        DEAD,
        ALIVE,
        AGING1,
        AGING2,
        AGING3
    };

    struct Cell
    {
        enum state;
        SDL_Color color;
    };
}