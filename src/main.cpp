#include <iostream>
#include <SDL3/SDL.h>
#include "imgui.h"
#include "imgui_impl_sdl3.h"
#include "imgui_impl_sdlgpu3.h"
#include "foo.hpp"
#include "cell.hpp"
#include "window.hpp"
#include "imgui_manager.hpp"
#include "event_handler.hpp"
#include "renderer.hpp"
#include <string>
#include <vector>
#include <random>

int main()
{
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dist(0, 255);

    int status = 0;
    Window window(1280, 720, "Cellular Automata");
    status += window.InitWindow();
    status += window.CreateWindow(SDL_WINDOW_RESIZABLE | SDL_WINDOW_HIDDEN | SDL_WINDOW_HIGH_PIXEL_DENSITY);
    status += window.CreateGPUDevice();
    status += window.ClaimWindowForGPUDevice();
    status += window.SetGPUSwapchainParams(SDL_GPU_SWAPCHAINCOMPOSITION_SDR, SDL_GPU_PRESENTMODE_VSYNC);
    if (status != 0)
    {
        std::cerr << "Error: window setup failed\n";
        return -1;
    }

    {
        ImGuiManager imgui_manager(&window);
        imgui_manager.CreateAndSetupContext();
        ImGuiIO* io = imgui_manager.GetIO();
        imgui_manager.SetupScaling();
        imgui_manager.SetupBackends();

        EventHandler event_handler(&window, &imgui_manager);
        SDL_ShowWindow(window.GetSDLWindow());
        
        Renderer renderer(&window);

        window.StartRun();
        while (window.IsRunning())
        {
            event_handler.ProcessEvents(); 
            if (SDL_GetWindowFlags(window.GetSDLWindow()) & SDL_WINDOW_MINIMIZED)
            {
                SDL_Delay(10);
                continue;
            }

            imgui_manager.BeginFrame();
            imgui_manager.Build();
            imgui_manager.EndFrame();
            
            auto* draw_data = imgui_manager.GetDrawData();
            const bool is_minimized = (draw_data->DisplaySize.x <= 0.0f || draw_data->DisplaySize.y <= 0.0f);

            /*
                rects.BeginFrame();
                {
                for loop
                    Rect r {};
                    rectc.Add(r, fbW, fbH)
                }
            */
            renderer.AcquireCommandBuffer();
            /*
                rects.Upload(command_buffer)
            */
           imgui_manager.Prepare(renderer.GetCommandBuffer());

            // Uint32 fbW=0, fbH=0;
            renderer.AcquireSwapchainTexture();
            auto* swapchain_texture = renderer.GetSwapchainTexture();
            
            if (swapchain_texture != nullptr && !is_minimized)
            {
                // Setup and start a render pass
                renderer.SetColorTargetInfo(ImVec4(0.45f, 0.55f, 0.60f, 1.00f));
                renderer.BeginRenderPass();
                /*
                rects.Draw(render_pass);
                */
                imgui_manager.Draw(renderer.GetRenderPass(), renderer.GetCommandBuffer());
                renderer.EndRenderPass();
            }
            renderer.SubmitCommandBuffer();
        }
    }
    
    return 0;
}