#pragma once

#include <algorithm>

class SimulationClock
{
public:
    int Advance(double elapsed_seconds, int generations_per_second)
    {
        // Bound catch-up after a stall so rendering and input can keep up.
        constexpr double max_generations_per_frame = 4.0;
        m_generations_due = std::min(m_generations_due + elapsed_seconds * generations_per_second,
                                     max_generations_per_frame);
        const int steps = static_cast<int>(m_generations_due);
        m_generations_due -= steps;
        return steps;
    }

    void Reset()
    {
        m_generations_due = 0.0;
    }

private:
    double m_generations_due = 0.0;
};
