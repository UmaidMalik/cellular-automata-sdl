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

        bool show_demo_window = imgui_manager.m_show_demo_window;
        bool show_another_window = false;
        bool set_fullscreen = false;
        ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);
        bool is_running = true;
        while (is_running)
        {
        SDL_Event event;
        while (SDL_PollEvent(&event))
        {
            imgui_manager.ProcessEvent(&event);
            if (event.type == SDL_EVENT_QUIT) 
            {
                is_running = false;
            }
            if (event.type == SDL_EVENT_WINDOW_CLOSE_REQUESTED && event.window.windowID == SDL_GetWindowID(window.GetSDLWindow()))
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
        imgui_manager.StartNewFrame();

        // 1. Show the big demo window (Most of the sample code is in ImGui::ShowDemoWindow()! You can browse its code to learn more about Dear ImGui!).
        if (show_demo_window)
        {
            imgui_manager.ShowDemoWindow();
        }
        // 2. Show a simple window that we create ourselves. We use a Begin/End pair to create a named window.
        {
            static float f = 0.0f;
            static int8_t counter = 120;

            ImGui::Begin("Hello, world!");                          // Create a window called "Hello, world!" and append into it.

            ImGui::Text("This is some useful text.");               // Display some text (you can use a format strings too)
            ImGui::Checkbox("Demo Window", &show_demo_window);      // Edit bools storing our window open/close state
            ImGui::Checkbox("Another Window", &show_another_window);
            if (ImGui::Checkbox("Fullscreen", &set_fullscreen))
            {
                window.SetFullscreen(set_fullscreen);
            }

            ImGui::SliderFloat("float", &f, 0.0f, 77.0f);            // Edit 1 float using a slider from 0.0f to 1.0f
            ImGui::ColorEdit3("clear color", (float*)&clear_color); // Edit 3 floats representing a color
        

            if (ImGui::Button("Button"))                            // Buttons return true when clicked (most widgets return true when edited/activated)
                counter++;
            ImGui::SameLine();
            ImGui::Text("counter = %d", counter);

            ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / io->Framerate, io->Framerate);
            ImGui::End();
        }
        // 3. Show another simple window.
        if (show_another_window)
        {
            ImGui::Begin("Another Window", &show_another_window);   // Pass a pointer to our bool variable (the window will have a closing button that will clear the bool when clicked)
            ImGui::Text("Hello from another window!");
            if (ImGui::Button("Close Me"))
                show_another_window = false;
            ImGui::End();
        }

        // Rendering
        // (Your code clears your framebuffer, renders your other stuff etc.)
        imgui_manager.Render();
        
        auto* draw_data = imgui_manager.GetDrawData();
        const bool is_minimized = (draw_data->DisplaySize.x <= 0.0f || draw_data->DisplaySize.y <= 0.0f);

        SDL_GPUCommandBuffer* command_buffer = SDL_AcquireGPUCommandBuffer(window.GetGPUDevice()); // Acquire a GPU command buffer

        SDL_GPUTexture* swapchain_texture;
        SDL_WaitAndAcquireGPUSwapchainTexture(command_buffer, window.GetSDLWindow(), &swapchain_texture, nullptr, nullptr); // Acquire a swapchain texture

        if (swapchain_texture != nullptr && !is_minimized)
        {
            // This is mandatory: call ImGui_ImplSDLGPU3_PrepareDrawData() to upload the vertex/index buffer!
            ImGui_ImplSDLGPU3_PrepareDrawData(draw_data, command_buffer);

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
            // Render ImGui
            ImGui_ImplSDLGPU3_RenderDrawData(draw_data, command_buffer, render_pass);
            SDL_EndGPURenderPass(render_pass);
        }

        // Submit the command buffer
        SDL_SubmitGPUCommandBuffer(command_buffer);
    
        }
    }
    // Cleanup
    // [If using SDL_MAIN_USE_CALLBACKS: all code below would likely be your SDL_AppQuit() function]
    SDL_WaitForGPUIdle(window.GetGPUDevice());

    

    SDL_ReleaseWindowFromGPUDevice(window.GetGPUDevice(), window.GetSDLWindow());
    SDL_DestroyGPUDevice(window.GetGPUDevice());
    SDL_DestroyWindow(window.GetSDLWindow());

    return 0;
}