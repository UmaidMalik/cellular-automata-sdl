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
#include "automata_simulator.hpp"
#include "math.hpp"
#include <string>
#include <vector>
#include <random>
#include <regex>
#include "utility.hpp"

int main(int argc, char* argv[])
{
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dist(0, 255);

    AutomataSimulator auto_sim(7, 7);
    auto_sim.DebugPrint();

    

    int id1 = auto_sim.GetIndex(math::ivec2{1,3});
    int id2 = auto_sim.GetIndex(math::ivec2{2,3});
    int id3 = auto_sim.GetIndex(math::ivec2{3,3});
    Cell cell(1, HSVColor{ 0.5f, 1.0f, 1.0f });

    auto& grid = *auto_sim.GetGrid(); 
    grid[id1] = cell;
    grid[id2] = cell;
    grid[id3] = cell;
    //auto_sim.Debug_SetConwaysGameOfLifeRule();
    std::string str1 = "0-9";
    std::string str2 = " 0-9";
    auto_sim.SetRange(1);
    auto_sim.SetMaxStates(2);
    auto_sim.ParseRule(str1, true);
    auto_sim.ParseRule(str2, false);
    auto_sim.DebugPrint();
    auto_sim.Step();
    auto_sim.DebugPrint();
    auto_sim.Step();
    auto_sim.DebugPrint();

    /*
        m_range = 1;
    m_max_states = 2;
    m_born.clear();
    m_survive.clear();
    m_born.insert(3);
    m_survive.insert(2);
    m_survive.insert(3);
    */

    //std::string my_test = "      7-3,4   -  6,   7,9  ";
    //auto_sim.ParseRule(my_test, true);

    int status = 0;
    Window window(1280, 720, "Cellular Automata");
    if (!window.SetupWindow())
    {
        std::cerr << "Error: window setup failed\n";
        SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Error", "Ran into an error during window setup :(", nullptr);
        return 1;
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
    
            // CA Create GPU Texture
            SDL_GPUTexture* cellular_automata_texture = nullptr;
            {
                SDL_GPUTextureCreateInfo texture_info{};
                texture_info.type = SDL_GPU_TEXTURETYPE_2D;
                texture_info.format = SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM;
                texture_info.usage = SDL_GPU_TEXTUREUSAGE_SAMPLER;
                texture_info.width = auto_sim.GetWidth();
                texture_info.height = auto_sim.GetHeight();
                texture_info.layer_count_or_depth = 1;
                texture_info.num_levels = 1;
                cellular_automata_texture = SDL_CreateGPUTexture(window.GetGPUDevice(), &texture_info);
                // will handle if nullptr after....
            }

            // CA Create GPU Transfer Buffer
            SDL_GPUTransferBuffer* transfer_buffer = nullptr;
            {
                SDL_GPUTransferBufferCreateInfo upload{};
                upload.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD;
                upload.size = auto_sim.GetWidth() * auto_sim.GetHeight() * 4;
                transfer_buffer = SDL_CreateGPUTransferBuffer(window.GetGPUDevice(), &upload); 
                // will hanlde if nullptr after...
            }

            // for crisp scaling in imgui
            SDL_GPUSampler* nearest_sampler = nullptr;
            {
                SDL_GPUSamplerCreateInfo sampler_info{};
                sampler_info.min_filter = SDL_GPU_FILTER_NEAREST;
                sampler_info.mag_filter = SDL_GPU_FILTER_NEAREST;
                sampler_info.mipmap_mode = SDL_GPU_SAMPLERMIPMAPMODE_NEAREST;
                sampler_info.address_mode_u = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE; 
                sampler_info.address_mode_v = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE;
                sampler_info.address_mode_w = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE;
                nearest_sampler = SDL_CreateGPUSampler(window.GetGPUDevice(), &sampler_info);
            }
            
            renderer.AcquireCommandBuffer();
            SDL_GPUCommandBuffer* cmd = renderer.GetCommandBuffer();

           
      
            

           imgui_manager.Prepare(renderer.GetCommandBuffer());
            // Uint32 fbW=0, fbH=0;
            renderer.AcquireSwapchainTexture();
            auto* swapchain_texture = renderer.GetSwapchainTexture();
            if (swapchain_texture != nullptr && !is_minimized)
            {
                renderer.SetColorTargetInfo(math::RGBAColor(0.0f, 0.0f, 0.0f, 1.0f));
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

