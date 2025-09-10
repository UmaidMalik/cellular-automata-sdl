#include <iostream>
#include <SDL3/SDL.h>
#include <SDL3/SDL_opengl.h>
#include "imgui.h"
#include "imgui_impl_sdl3.h"
#include "imgui_impl_sdlgpu3.h"
#include "foo.hpp"
#include "cell.hpp"
#include <string>
#include <random>

int main()
{
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dist(0, 255);

    if (SDL_Init(SDL_INIT_VIDEO) < 0)
    {
        std::cerr << "Failed to initialize SDL: " << SDL_GetError() << "\n";
        return -1;
    }

    SDL_Window* window = SDL_CreateWindow("Colorful Cells", 800, 600, SDL_WINDOW_OPENGL);
    if (!window)
    {
        std::cerr << "Failed to create window: " << SDL_GetError() << "\n";
        return -1;
    }

    SDL_Renderer* renderer = SDL_CreateRenderer(window, "OpenGL");
    if (!renderer)
    {
        std::cerr << "Failed to create renderer: " << SDL_GetError() << "\n";
        SDL_DestroyWindow(window);
        return -1;
    }
    
    bool isRunning = true;
    while (isRunning) 
    {
        SDL_Event event;
        while (SDL_PollEvent(&event))
        {
            if (event.type == SDL_EVENT_QUIT) 
            {
                isRunning = false;
            }
        }
        
        // Rendering code would go here
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);

        SDL_SetRenderDrawColor(renderer, 40, 34, 100, 255);
        SDL_FRect rect = {100.0f, 100.0f, 200.0f, 150.0f};
        SDL_RenderFillRect(renderer, &rect);

        SDL_SetRenderDrawColor(renderer, dist(gen), dist(gen), dist(gen), dist(gen));
        SDL_FRect rect2 = {(float)dist(gen), (float)dist(gen), (float)dist(gen), (float)dist(gen)};
        SDL_RenderFillRect(renderer, &rect2);

        SDL_RenderPresent(renderer);
        
        SDL_Delay(500); // ~60 FPS
    }

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}