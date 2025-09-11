#pragma once

#include <SDL3/SDL.h>

class Window
{
    private:
        size_t m_width;
        size_t m_height;
        const char* m_window_title;
        SDL_Window* m_sdl_window;
    public:
        Window(size_t width, size_t height, const char* title);
        ~Window();
        int InitWindow();
        SDL_Window* GetSDLWindow() const;
};