#include "window.hpp"
#include <iostream>

Window::Window(int width, int height, const char* title)
    : m_width(width), m_height(height), m_window_title(title)
{
    std::cout << "Window created\n";
}

Window::~Window()
{
    if (m_sdl_window)
    {
        SDL_DestroyWindow(m_sdl_window);
        m_sdl_window = nullptr;
    }
}

int Window::InitWindow()
{
    if (!SDL_Init(SDL_INIT_VIDEO))
    {
        SDL_Log("Failed to initialize SDL: %s", SDL_GetError());
        return -1;
    }
    SDL_Log("SDL initialized successfully");

    m_sdl_window = SDL_CreateWindow(m_window_title, m_width, m_height, SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE);

    if (!m_sdl_window)
    {
        std::cerr << "Failed to create window: " << SDL_GetError() << "\n";
        return -1;
    }
    SDL_Log("Window created successfully: %s", m_window_title);
    return 0;
}

SDL_Window* Window::GetSDLWindow() const
{ 
    return m_sdl_window; 
}

void Window::SetFullscreen(bool enable)
{
    SDL_SetWindowFullscreen(m_sdl_window, enable);
}