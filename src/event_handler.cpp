#include "event_handler.hpp"
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