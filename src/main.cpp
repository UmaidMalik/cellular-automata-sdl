#include <iostream>
#include <SDL3/SDL.h>
#include <SDL3/SDL_gpu.h>
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

static std::vector<uint8_t> ReadFile(const char* path);

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

    int status = 0;
    Window window(1280, 720, "Cellular Automata");
    if (!window.SetupWindow())
    {
        std::cerr << "Error: window setup failed\n";
        SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Error", "Ran into an error during window setup :(", nullptr);
        return 1;
    }

    SDL_GPUShaderFormat formats = SDL_GetGPUShaderFormats(window.GetGPUDevice());
    auto load = [&](const char* path, SDL_GPUShaderStage stage, SDL_GPUShaderFormat format)
    {
        std::vector<uint8_t> bytes = ReadFile(path);
        SDL_GPUShaderCreateInfo info{};
        info.stage = stage;
        info.format = format;
        info.entrypoint = "main";
        info.code = bytes.data();
        info.code_size = bytes.size();
        return SDL_CreateGPUShader(window.GetGPUDevice(), &info);
    };

    SDL_GPUShader* vertex_shader = nullptr;
    SDL_GPUShader* fragment_shader = nullptr;

    if (formats & SDL_GPU_SHADERFORMAT_SPIRV)
    {
        vertex_shader = load("shaders/build/grid_fullscreen_triangle.vert.spv", SDL_GPU_SHADERSTAGE_VERTEX, SDL_GPU_SHADERFORMAT_SPIRV);
        fragment_shader = load("shaders/build/grid_sample_rgba8.frag.spv", SDL_GPU_SHADERSTAGE_FRAGMENT, SDL_GPU_SHADERFORMAT_SPIRV);
    }
    else if (formats & SDL_GPU_SHADERFORMAT_DXIL)
    {
        vertex_shader = load("shaders/build/grid_fullscreen_triangle.vs.dxil", SDL_GPU_SHADERSTAGE_VERTEX, SDL_GPU_SHADERFORMAT_DXIL);
        fragment_shader = load("shaders/build/grid_sample_rgba8.ps.dxil", SDL_GPU_SHADERSTAGE_FRAGMENT, SDL_GPU_SHADERFORMAT_DXIL);
    }
    else if (formats & SDL_GPU_SHADERFORMAT_MSL)
    {
        vertex_shader = load("shaders/build/grid_fullscreen_triangle.vert.msl", SDL_GPU_SHADERSTAGE_VERTEX, SDL_GPU_SHADERFORMAT_MSL);
        fragment_shader = load("shaders/build/grid_sample_rgba8.frag.msl", SDL_GPU_SHADERSTAGE_FRAGMENT, SDL_GPU_SHADERFORMAT_MSL);
    }
    else {
        std::cerr << "No supported precompiled shader format found.\n";
        SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Error", "No supported precompiled shader format found :(", nullptr);
        return 1;
    }

    if (!vertex_shader || !fragment_shader)
    {
        std::cerr << "Vertex or fragment shader not loaded\n";
        SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Error", "Vertex or fragment shader not loaded :(", nullptr);
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

        SDL_GPUVertexInputState vertex_input_state{};
        SDL_GPUColorTargetDescription color_target{};
        color_target.format = renderer.GetSwapchainFormat();

        // pipeline
        SDL_GPUGraphicsPipelineCreateInfo pipeline_info{};
        pipeline_info.vertex_shader = vertex_shader;
        pipeline_info.fragment_shader = fragment_shader;
        pipeline_info.vertex_input_state = vertex_input_state;
        pipeline_info.primitive_type = SDL_GPU_PRIMITIVETYPE_TRIANGLELIST;
    
        SDL_GPUGraphicsPipeline* grid_pipeline = SDL_CreateGPUGraphicsPipeline(window.GetGPUDevice(), &pipeline_info);



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
            // will handle if nullptr after...
        }

        // --- will add textured-quad pipeline + vertex/index buffers (created once)
        // SDL_CreateGPUGraphicsPipeline(...), SDL_CreateGPUBuffer(...), etc.
        // (Bind pos+uv; fragment samples caTex with 'nearest')
        

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
    
            
            const auto& grid = auto_sim.GetGrid();
            int W = auto_sim.GetWidth();
            int H = auto_sim.GetHeight();
            std::vector<uint32_t> pixels(W * H);
            // will map cell to RGBA8

            auto map_color = [](const Cell& cell)->uint32_t
            {
                math::RGBColor color = math::HSVtoRGB(cell.color);
                uint8_t r = uint8_t(color.r * 255);
                uint8_t g = uint8_t(color.g * 255);
                uint8_t b = uint8_t(color.b * 255);
                uint8_t a = 255;
                return (uint32_t(r)) | (uint32_t(g)<<8) | (uint32_t(b)<<16) | (uint32_t(a)<<24);
            };

            const auto& grid_data = *auto_sim.GetGrid();
            for (int i = 0; i < grid_data.size(); i++)
            {
                pixels[i] = map_color(grid_data[i]);
            }
            
            renderer.AcquireCommandBuffer();
            SDL_GPUCommandBuffer* command_buffer = renderer.GetCommandBuffer();

            // upload pixels to ca texture in a COPY PASS
            void* dst = SDL_MapGPUTransferBuffer(window.GetGPUDevice(), transfer_buffer, true);
            memcpy(dst, pixels.data(), pixels.size() * sizeof(uint32_t));
            SDL_UnmapGPUTransferBuffer(window.GetGPUDevice(), transfer_buffer);

            SDL_GPUCopyPass* copy_pass = SDL_BeginGPUCopyPass(command_buffer);
            SDL_GPUTextureTransferInfo tti{};
            tti.transfer_buffer = transfer_buffer;
            tti.offset = 0;
            tti.pixels_per_row = 0;
            tti.rows_per_layer = 0;
            SDL_GPUTextureRegion dst_region{};
            dst_region.texture = cellular_automata_texture;
            dst_region.mip_level = 0;
            dst_region.layer = 0;
            dst_region.x=0; dst_region.y=0; dst_region.z=0;
            dst_region.w = auto_sim.GetWidth();
            dst_region.h = auto_sim.GetHeight();
            dst_region.d = 1;
            SDL_UploadToGPUTexture(copy_pass, &tti, &dst_region, true);
            SDL_EndGPUCopyPass(copy_pass);


            imgui_manager.Prepare(renderer.GetCommandBuffer());
            renderer.AcquireSwapchainTexture();
            auto* swapchain_texture = renderer.GetSwapchainTexture();
            if (swapchain_texture != nullptr && !is_minimized)
            {
                renderer.SetColorTargetInfo(math::RGBAColor(0.0f, 0.0f, 0.0f, 1.0f));
                renderer.BeginRenderPass();
                SDL_GPURenderPass* render_pass = renderer.GetRenderPass();
                
                // bind pipeline, verter buffer, and the ca texture + sampler at FS slot 0
                SDL_BindGPUGraphicsPipeline(render_pass, grid_pipeline);

                SDL_GPUTextureSamplerBinding texture_sampling{ .texture=cellular_automata_texture, .sampler=nearest_sampler };

                SDL_BindGPUFragmentSamplers(render_pass, 0, &texture_sampling, 1);

                SDL_DrawGPUPrimitives(render_pass, 3, 1, 0, 0);

                //SDL_GPUBufferBinding vertex_buffer{ .buffer=grid_vertex_buffer, .offset=0};
                //SDL_BindGPUVertexBuffers(render_pass, 0, &vertex_buffer, 1);

                
        
                imgui_manager.Draw(render_pass, renderer.GetCommandBuffer());
                renderer.EndRenderPass();
            }
            renderer.SubmitCommandBuffer();
        }
    }
    return 0;
}

static std::vector<uint8_t> ReadFile(const char* path)
{
    std::FILE* file = std::fopen(path, "rb");
    if (!file)
    {
        return {};
    }
    std::fseek(file, 0, SEEK_END);
    long size = ftell(file);
    std::fseek(file, 0, SEEK_SET);
    std::vector<uint8_t> buffer(size);
    std::fclose(file);
    return buffer;
}