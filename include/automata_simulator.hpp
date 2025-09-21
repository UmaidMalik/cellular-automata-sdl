#pragma once

#include "cell.hpp"
#include "stdint.h"
#include "math.hpp"
#include <iostream>
#include <vector>
#include <unordered_set>
#include <algorithm>
#include <string>
#include <sstream>
#include "utility.hpp"

using uset = std::unordered_set<int>;
using vector = std::vector<Cell>;

struct CountAndHue
{
    int count = 0;
    float hue = 0.0f;
};

class AutomataSimulator
{
    private:
        int m_width;
        int m_height;
        int m_range = 1;
        uset m_born;
        uset m_survive;
        int m_max_states = 2;
        vector m_grid;
        vector m_grid_next;
        float m_saturation = 1.0f;
        float m_value = 1.0f;
        float m_decay_rate = 0.0f;
    public:
        AutomataSimulator(int w, int h);
        ~AutomataSimulator();
        void DebugPrint();

        bool ParseRule(std::string& rule_string, bool is_born_rule);
        void SetRule(uset born, uset survive, int range, int max_states);

        void SetBornRule(uset born);
        void SetSurviveRule(uset survive);
        void SetRange(int range);
        void SetMaxStates(int max_states);

        uset GetBornRule();
        uset GetSurviveRule();
        int GetRange();
        int GetMaxStates();
        std::vector<Cell>* GetGrid();
        int GetWidth();
        int GetHeight();

        void Step();
        
        void ApplyAutomataRule(int idx, CountAndHue count_hue);
        CountAndHue GetNeighborCountAndAverageColor(int idx);
        math::ivec2 GetPosition(int idx);
        int GetIndex(math::ivec2 v);
        bool IsOutOfBounds(const math::ivec2& v);

        void Debug_SetConwaysGameOfLifeRule();
};

AutomataSimulator::AutomataSimulator(int w, int h)
    : m_width(w), m_height(h)
{
    m_grid.resize(m_width * m_height);
    m_grid_next.resize(m_width * m_height);
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

/*
    expected rule string format: 
    0-3,4-6,7,9
*/
bool AutomataSimulator::ParseRule(std::string& rule_string, bool is_born_rule)
{
    uset& rule_set = is_born_rule ? m_born : m_survive;
    rule_set.clear();
    util::remove_spaces(rule_string);
    std::regex pattern(R"(^\d+(-\d+)?(,\d+(-\d+)?)*$)");
    if (!std::regex_match(rule_string, pattern))
    {
        std::cerr << "Error: Invalid rule string" << "\n";
        return false;
    }
    auto rules = util::split_and_trim(rule_string, ',');
    for(auto& rule: rules)
    {
        std::cout << "[DEBUG]: <" << rule << ">\n";
        auto dash = rule.find('-');
        if (dash == std::string::npos)
        {
            rule_set.insert(std::stoi(rule));
        }
        else
        {
            int a = std::stoi(rule.substr(0, dash));
            int b = std::stoi(rule.substr(dash + 1));

            if (a > b)
            {
                std::swap(a, b);
            }

            for (size_t i = a; i <= b; i++)
            {
                rule_set.insert(i);
            }
        }
    }



    return true;
}

void AutomataSimulator::SetRule(uset born, uset survive, int range, int max_states)
{
    m_born = born;
    m_survive = survive;
    m_range = range;
    m_max_states = max_states;
}

void AutomataSimulator::SetBornRule(uset born)
{
    m_born = born;
}

void AutomataSimulator::SetSurviveRule(uset survive)
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

uset AutomataSimulator::GetBornRule()
{
    return m_born;
}

uset AutomataSimulator::GetSurviveRule()
{
    return m_survive;
}

int AutomataSimulator::GetRange()
{
    return m_range;
}

int AutomataSimulator::GetMaxStates()
{
    return m_max_states;
}

vector* AutomataSimulator::GetGrid()
{
    return &m_grid;
}

void AutomataSimulator::Step()
{
    CountAndHue count_hue;
    int num_alive_neighbors = 0;
    for (size_t i = 0; i < m_grid.size(); i++)
    {
        count_hue = GetNeighborCountAndAverageColor(i);
        ApplyAutomataRule(i, count_hue);
    }
    std::swap(m_grid, m_grid_next);
}

CountAndHue AutomataSimulator::GetNeighborCountAndAverageColor(int idx)
{
    CountAndHue count_hue;
    int x_coord = 0, y_coord = 0;
    math::ivec2 v = GetPosition(idx);
    int i = v.x, j = v.y;
    int curr_idx;
    for (size_t x = i - m_range; x <= i + m_range; x++)
    {
        for (size_t y = j - m_range; y <= j + m_range; y++)
        {
            x_coord = ((x + m_width) % m_width);
            y_coord = ((y + m_height) % m_height);
            int curr_idx = GetIndex(math::ivec2{ x_coord, y_coord });

            if (IsOutOfBounds((math::ivec2){ x_coord, y_coord }))
            {
                continue;
            }
            if (!(x == i && y == j) && m_grid[curr_idx].state == static_cast<int>(CellState::ALIVE))
            {
                count_hue.count += 1;
                count_hue.hue += m_grid[curr_idx].color.h;
            }
        }
    }
    if (count_hue.count > 0)
    {
        count_hue.hue /= count_hue.count;
    }
    else
    {
        count_hue.hue = 0.0f;
    }
    return count_hue;
}

void AutomataSimulator::ApplyAutomataRule(int idx, CountAndHue count_hue)
{

    int cell_state_int = m_grid[idx].state;
    
    bool is_alive = (cell_state_int == static_cast<int>(CellState::ALIVE)) ? true : false; 
    bool is_dead = (cell_state_int == static_cast<int>(CellState::DEAD)) ? true : false;
    bool should_survive = is_alive && m_survive.find(count_hue.count) != m_survive.end(); 
    bool should_be_born = is_dead && m_born.find(count_hue.count) != m_born.end();

    if (should_be_born)
    {
        m_grid_next[idx].state = static_cast<int>(CellState::ALIVE);
        m_grid_next[idx].color.h = count_hue.hue;
        m_grid_next[idx].color.s = m_saturation;
        m_grid_next[idx].color.v = m_value;
    }
    else if (is_dead)
    {
        HSVColor color{ m_grid[idx].color.h, m_saturation, m_grid[idx].color.v * m_decay_rate };
        m_grid_next[idx].state = static_cast<int>(CellState::DEAD);
        m_grid_next[idx].color = color;
    }
    else if (should_survive)
    {
        m_grid_next[idx].state = static_cast<int>(CellState::ALIVE);
        m_grid_next[idx].color.h = count_hue.hue;
        m_grid_next[idx].color.s = m_saturation;
        m_grid_next[idx].color.v = m_value;
    }
    else
    {
        m_grid_next[idx].state =  ++cell_state_int % m_max_states;
        m_grid_next[idx].color.h = count_hue.hue;
        m_grid_next[idx].color.s = m_saturation;
        m_grid_next[idx].color.v = m_value;
    }
    
    
}

math::ivec2 AutomataSimulator::GetPosition(int idx)
{
	int x = idx % m_width;
	int y = idx / m_width;
	math::ivec2 v = { x, y };
	return v;
}

int AutomataSimulator::GetIndex(math::ivec2 v)
{
	int idx = 0;
	idx = v.x + (m_width * v.y);
	return idx;
}

bool AutomataSimulator::IsOutOfBounds(const math::ivec2& v)
{
    return (!(v.x >= 0 && v.x < m_width &&
        v.y >= 0 && v.y < m_height));
}

void AutomataSimulator::Debug_SetConwaysGameOfLifeRule()
{
    m_range = 1;
    m_max_states = 2;
    m_born.clear();
    m_survive.clear();
    m_born.insert(3);
    m_survive.insert(2);
    m_survive.insert(3);
}

int AutomataSimulator::GetWidth()
{
    return m_width;
}

int AutomataSimulator::GetHeight()
{
    return m_height;
}