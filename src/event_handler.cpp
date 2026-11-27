#include "event_handler.hpp"
#include <algorithm>
#include <iostream>
#include <SDL3/SDL.h>
#include "imgui_manager.hpp"
#include "window.hpp"

EventHandler::EventHandler(Window* window, ImGuiManager* imgui_manager)
    : m_window(window), m_imgui_manager(imgui_manager)
{

}

EventHandler::~EventHandler()
{
    m_window = nullptr;
    m_imgui_manager = nullptr;
    std::cout << "~EventHandler() destructor called and exited\n";
}

void EventHandler::ProcessTouch(const SDL_TouchFingerEvent& event)
{
    if (event.windowID != SDL_GetWindowID(m_window->GetSDLWindow()))
        return;

    const auto contact = std::ranges::find_if(m_touches, [&](const TouchContact& touch) {
        return touch.device == event.touchID && touch.id == event.fingerID;
    });
    const SDL_FPoint position{event.x, event.y};
    if (event.type == SDL_EVENT_FINGER_DOWN && contact == m_touches.end())
    {
        if (m_touches.empty())
            m_multitouch = false;
        else
            m_multitouch = true;
        m_touches.push_back({event.touchID, event.fingerID, position, position});
    }
    else if (contact != m_touches.end())
    {
        if (event.type == SDL_EVENT_FINGER_UP || event.type == SDL_EVENT_FINGER_CANCELED)
            m_touches.erase(contact);
        else
            contact->position = position;
    }
}

void EventHandler::ProcessEvents()
{
    while (SDL_PollEvent(&m_event))
    {
        m_imgui_manager->ProcessEvent(&m_event);
        switch (m_event.type)
        {
            case SDL_EVENT_QUIT:
                m_window->EndRun();
                break;
            case SDL_EVENT_WINDOW_CLOSE_REQUESTED:
                if (m_event.window.windowID == SDL_GetWindowID(m_window->GetSDLWindow()))
                {
                    m_window->EndRun();
                }
                break;
            case SDL_EVENT_FINGER_DOWN:
            case SDL_EVENT_FINGER_MOTION:
            case SDL_EVENT_FINGER_UP:
            case SDL_EVENT_FINGER_CANCELED:
                ProcessTouch(m_event.tfinger);
                break;
            case SDL_EVENT_WINDOW_FOCUS_LOST:
                if (m_event.window.windowID == SDL_GetWindowID(m_window->GetSDLWindow()))
                    m_touches.clear();
                break;
            case SDL_EVENT_KEY_DOWN:
                if (m_event.key.scancode == SDL_SCANCODE_ESCAPE && !m_event.key.repeat &&
                    m_event.key.windowID == SDL_GetWindowID(m_window->GetSDLWindow()))
                {
                    m_window->SetFullscreen(false);
                }
                break;
        }
    }
}