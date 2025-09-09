#include <iostream>
#include <SDL3/SDL.h>
#include <SDL3/SDL_opengl.h>
#include "imgui.h"
#include "foo.hpp"

int main() {
    std::cout << "Hello, world!\n";
    the::foo();
    std::cout << "Hello, world!\n";
    SDL_Window *window = SDL_CreateWindow("Hello World", 800, 600, SDL_WINDOW_OPENGL);
    if (!window) {
        std::cerr << "Failed to create window: " << SDL_GetError() << std::endl;
        return -1;
    }
    while (true) {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_EVENT_QUIT) {
                SDL_DestroyWindow(window);
                return 0;
            }
        }
        // Rendering code would go here
    }
    return 0;
}