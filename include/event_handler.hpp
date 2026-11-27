#pragma once

#include <SDL3/SDL.h>
#include "imgui_manager.hpp"
#include "window.hpp"
#include <span>
#include <vector>

struct TouchContact
{
    SDL_TouchID device;
    SDL_FingerID id;
    SDL_FPoint origin;
    SDL_FPoint position;
};

class EventHandler
{
    private:
        SDL_Event m_event;
        std::vector<TouchContact> m_touches;
        bool m_multitouch = false;
        void ProcessTouch(const SDL_TouchFingerEvent& event);
        Window* m_window = nullptr;
        ImGuiManager* m_imgui_manager = nullptr;
    public:
        EventHandler(Window* window, ImGuiManager* imgui_manager);
        ~EventHandler();
        void ProcessEvents();
        std::span<const TouchContact> Touches() const { return m_touches; }
        bool MultitouchSequence() const { return m_multitouch; }
};