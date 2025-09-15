#pragma once

#include "cell.hpp"
#include "stdint.h"
#include "math.hpp"
#include <iostream>
#include <vector>

class AutomataSimulator
{
    private:
        uint16_t m_width;
        uint16_t m_height;
        uint8_t m_range;
        std::vector<Cell> m_grid;
    public:
        AutomataSimulator(uint16_t w, uint16_t h, uint8_t r);
        ~AutomataSimulator();
        void DebugPrint();
        static math::ivec2 GetPosition(int idx, uint16_t width);
        static int GetIndex(math::ivec2 v, uint16_t width);
};

AutomataSimulator::AutomataSimulator(uint16_t w, uint16_t h, uint8_t r)
    : m_width(w), m_height(h), m_range(r)
{
    m_grid.resize(m_width * m_height);
}

AutomataSimulator::~AutomataSimulator()
{
    std::cout << "~AutomataSimulator() destructor called and exited\n";
}

void AutomataSimulator::DebugPrint()
{
    std::cout << "grid size: " << m_grid.size();
    for (size_t i = 0; i < m_grid.size(); i++)
    {
        if (i % m_width == 0)
        {
            std::cout << "\n";
        }
        std::cout << m_grid[i] << " ";
    }
    std::cout << "\n";
}

math::ivec2 AutomataSimulator::GetPosition(int idx, uint16_t width)
{
	int x = idx % width;
	int y = idx / width;
	math::ivec2 v = { x, y };
	return v;
}

int AutomataSimulator::GetIndex(math::ivec2 v, uint16_t width)
{
	int idx = 0;
	idx = v.x + (width * v.y);
	return idx;
}