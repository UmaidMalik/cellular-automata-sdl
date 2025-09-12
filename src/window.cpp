#include "window.hpp"
#include <SDL3/SDL.h>
#include <iostream>

Window::Window(int width, int height, const char* title)
    : m_width(width), m_height(height), m_window_title(title)
{
    m_main_scale = SDL_GetDisplayContentScale(SDL_GetPrimaryDisplay());
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
    if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_GAMEPAD))
    {
        SDL_Log("Error: SDL_Init(): %s\n", SDL_GetError());
        return -1;
    }
    SDL_Log("SDL initialized successfully");

    SDL_WindowFlags window_flags = SDL_WINDOW_RESIZABLE | SDL_WINDOW_HIDDEN | SDL_WINDOW_HIGH_PIXEL_DENSITY;
    m_main_scale = SDL_GetDisplayContentScale(SDL_GetPrimaryDisplay());
    m_sdl_window = SDL_CreateWindow(m_window_title, (int)(m_width * m_main_scale), (int)(m_height * m_main_scale), window_flags);

    if (!m_sdl_window)
    {
        SDL_Log("Error: SDL_CreateWindow(): %s\n", SDL_GetError()) ;
        return -1;
    }
    SDL_Log("Window created successfully: %s", m_window_title);
    SDL_SetWindowPosition(m_sdl_window, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED);
    SDL_ShowWindow(m_sdl_window);
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

float Window::GetMainScale()
{
    return m_main_scale;
}