#pragma once

#include <SDL3/SDL.h>

class Window
{
    private:
        int m_width;
        int m_height;
        const char* m_window_title;
        SDL_Window* m_sdl_window;
        float m_main_scale;
    public:
        Window(int width, int height, const char* title);
        ~Window();
        int InitWindow();
        SDL_Window* GetSDLWindow() const;
        void SetFullscreen(bool enable);
        float GetMainScale();
};