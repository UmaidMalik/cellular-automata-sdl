#include <iostream>
#include <SDL3/SDL.h>
#include <SDL3/SDL_opengl.h>
#include "imgui.h"
#include "imgui_impl_sdl3.h"
#include "imgui_impl_sdlgpu3.h"
#include "foo.hpp"
#include "cell.hpp"
#include "game.hpp"
#include "window.hpp"
#include <string>
#include <random>

int main()
{
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dist(0, 255);

    Window window(800, 600, "Cellular Automata");
    window.InitWindow();

    SDL_Renderer* renderer = SDL_CreateRenderer(window.GetSDLWindow(), "OpenGL");
    if (!renderer)
    {
        std::cerr << "Failed to create renderer: " << SDL_GetError() << "\n";
        SDL_DestroyWindow(window.GetSDLWindow());
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
    SDL_Quit();
    return 0;
}