#pragma once

#include <algorithm>

class SimulationClock
{
public:
    int Advance(double elapsed_seconds, int generations_per_second, int max_generations_per_frame = 4)
    {
        // Bound catch-up after a stall so rendering and input can keep up.
        m_generations_due = std::min(m_generations_due + elapsed_seconds * generations_per_second,
                                     static_cast<double>(max_generations_per_frame + 1));
        const int steps = std::min(static_cast<int>(m_generations_due), max_generations_per_frame);
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

class LowFrameRateWatchdog
{
public:
    bool Update(double frame_seconds, bool running)
    {
        constexpr double slow_frame = 1.0 / 15.0;
        if (!running || frame_seconds <= slow_frame)
        {
            Reset();
            return false;
        }
        m_slow_seconds += frame_seconds;
        return ++m_slow_frames >= 2 && m_slow_seconds >= 3.0;
    }

    void Reset()
    {
        m_slow_seconds = 0.0;
        m_slow_frames = 0;
    }

private:
    double m_slow_seconds = 0.0;
    int m_slow_frames = 0;
};
