#include <iostream>
#include <SDL3/SDL.h>
#include "imgui.h"
#include "imgui_impl_sdl3.h"
#include "imgui_impl_sdlgpu3.h"
#include "foo.hpp"
#include "cell.hpp"
#include "window.hpp"
#include "imgui_manager.hpp"
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

    // Setup Dear ImGui context
    {
        ImGuiManager imgui_manager(&window);
        imgui_manager.CreateAndSetupContext();
        ImGuiIO* io = imgui_manager.GetIO();
        imgui_manager.SetupScaling();
        imgui_manager.SetupBackends();

        SDL_ShowWindow(window.GetSDLWindow());

        ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);
        bool is_running = true;
        SDL_Event event;
        while (is_running)
        {
            while (SDL_PollEvent(&event))
            {
                imgui_manager.ProcessEvent(&event);
                if (event.type == SDL_EVENT_QUIT) 
                {
                    is_running = false;
                }
                if(event.type == SDL_EVENT_WINDOW_CLOSE_REQUESTED &&
                    event.window.windowID == SDL_GetWindowID(window.GetSDLWindow()))
                {
                    is_running = false;
                }

            }

            // [If using SDL_MAIN_USE_CALLBACKS: all code below would likely be your SDL_AppIterate() function]
            if (SDL_GetWindowFlags(window.GetSDLWindow()) & SDL_WINDOW_MINIMIZED)
            {
                SDL_Delay(10);
                continue;
            }

            // (After event loop)
            // Start the Dear ImGui frame
            imgui_manager.BeginFrame();
            imgui_manager.Build();
            // (Your code clears your framebuffer, renders your other stuff etc.)
            imgui_manager.EndFrame();
            
            auto* draw_data = imgui_manager.GetDrawData();
            const bool is_minimized = (draw_data->DisplaySize.x <= 0.0f || draw_data->DisplaySize.y <= 0.0f);

            SDL_GPUCommandBuffer* command_buffer = SDL_AcquireGPUCommandBuffer(window.GetGPUDevice());
            SDL_GPUTexture* swapchain_texture;
            SDL_WaitAndAcquireGPUSwapchainTexture(command_buffer, window.GetSDLWindow(), &swapchain_texture, nullptr, nullptr);

            if (swapchain_texture != nullptr && !is_minimized)
            {
                imgui_manager.Prepare(command_buffer);
                // Setup and start a render pass
                
                SDL_GPUColorTargetInfo target_info = {};
                target_info.texture = swapchain_texture;
                target_info.clear_color = SDL_FColor { clear_color.x, clear_color.y, clear_color.z, clear_color.w };
                target_info.load_op = SDL_GPU_LOADOP_CLEAR;
                target_info.store_op = SDL_GPU_STOREOP_STORE;
                target_info.mip_level = 0;
                target_info.layer_or_depth_plane = 0;
                target_info.cycle = false;
                
                SDL_GPURenderPass* render_pass = SDL_BeginGPURenderPass(command_buffer, &target_info, 1, nullptr);
                imgui_manager.Draw(render_pass, command_buffer);
                SDL_EndGPURenderPass(render_pass);
            }

            // Submit the command buffer
            SDL_SubmitGPUCommandBuffer(command_buffer);
    
        }
    }
    
    return 0;
}