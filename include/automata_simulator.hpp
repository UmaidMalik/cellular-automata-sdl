#pragma once

#include "cell.hpp"
#include "stdint.h"
#include "math.hpp"
#include <iostream>
#include <vector>

class AutomataSimulator
{
    private:
        int m_width;
        int m_height;
        int m_range;
        int m_born;
        int m_survive;
        int m_max_states;
        std::vector<Cell> m_grid;
    public:
        AutomataSimulator(int w, int h);
        ~AutomataSimulator();
        void DebugPrint();

        void SetRule(int born, int survive, int range, int max_states);

        void SetBornRule(int born);
        void SetSurviveRule(int survive);
        void SetRange(int range);
        void SetMaxStates(int max_states);

        int GetBornRule();
        int GetSurviveRule();
        void SetRange(int range);
        void SetMaxStates(int max_states);

        void Step();
        void CheckNeighbors(int idx);
        static math::ivec2 GetPosition(int idx, int width);
        static int GetIndex(math::ivec2 v, int width);
};

AutomataSimulator::AutomataSimulator(int w, int h)
    : m_width(w), m_height(h)
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

void AutomataSimulator::SetRule(int born, int survive, int range, int max_states)
{
    m_born = born;
    m_survive = survive;
    m_range = range;
    m_max_states = max_states;
}

void AutomataSimulator::SetBornRule(int born)
{
    m_born = born;
}

void AutomataSimulator::SetSurviveRule(int survive)
{
    m_survive = survive;
}

void AutomataSimulator::SetRange(int range)
{
    m_range = range;
}

void AutomataSimulator::SetMaxStates(int max_states)
{
    m_max_states = max_states;
}

int AutomataSimulator::GetBornRule()
{
    return m_born;
}

int AutomataSimulator::GetSurviveRule()
{
    return m_survive;
}

void AutomataSimulator::Step()
{

}

void AutomataSimulator::CheckNeighbors(int idx)
{
    math::ivec2 grid_pos = GetPosition(idx, m_width);
    int x = grid_pos.x, y = grid_pos.y;

}

math::ivec2 AutomataSimulator::GetPosition(int idx, int width)
{
	int x = idx % width;
	int y = idx / width;
	math::ivec2 v = { x, y };
	return v;
}

int AutomataSimulator::GetIndex(math::ivec2 v, int width)
{
	int idx = 0;
	idx = v.x + (width * v.y);
	return idx;
}