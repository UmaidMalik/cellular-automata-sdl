#pragma once

#include <SDL3/SDL.h>
#include "imgui_manager.hpp"
#include "window.hpp"

class EventHandler
{
    private:
        SDL_Event m_event;
        Window* m_window = nullptr;
        ImGuiManager* m_imgui_manager = nullptr;
    public:
        EventHandler(Window* window, ImGuiManager* imgui_manager);
        ~EventHandler();
        void ProcessEvents();
};