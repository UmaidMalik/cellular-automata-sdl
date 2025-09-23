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
#include <cstring>

static std::vector<uint8_t> ReadFile(const char* path);

int main(int argc, char* argv[])
{
    //std::random_device rd;
    //std::mt19937 gen(rd());
    //std::uniform_int_distribution<> dist(0, 255);

    AutomataSimulator auto_sim(64, 36);
    //auto_sim.DebugPrint();

    

    int id1 = auto_sim.GetIndex(math::ivec2{1,3});
    int id2 = auto_sim.GetIndex(math::ivec2{2,3});
    int id3 = auto_sim.GetIndex(math::ivec2{3,3});
    Cell cell(1, HSVColor{ 0.5f, 1.0f, 1.0f });

    auto& grid = *auto_sim.GetGrid(); 
    grid[id1] = cell;
    grid[id2] = cell;
    grid[id3] = cell;
    //auto_sim.Debug_SetConwaysGameOfLifeRule();
    std::string str1 = "3";
    std::string str2 = "2-3";
    auto_sim.SetRange(1);
    auto_sim.SetMaxStates(2);
    auto_sim.ParseRule(str1, true);
    auto_sim.ParseRule(str2, false);
    //auto_sim.DebugPrint();
    auto_sim.Step();
    //auto_sim.DebugPrint();
    auto_sim.Step();
    //auto_sim.DebugPrint();

    SDL_SetLogPriorities(SDL_LOG_PRIORITY_VERBOSE);
    SDL_SetHint(SDL_HINT_DEBUG_LOGGING, "1");
     

    int status = 0;
    Window window(1280, 720, "Cellular Automata");
    if (!window.SetupWindow())
    {
        std::cerr << "Error: window setup failed\n";
        SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Error", "Ran into an error during window setup :(", nullptr);
        return 1;
    }

    SDL_GPUShaderFormat formats = SDL_GetGPUShaderFormats(window.GetGPUDevice());
    auto load = [&]
    (
        const char* path,
        SDL_GPUShaderStage stage,
        SDL_GPUShaderFormat format,
        uint32_t num_samplers = 0,
        uint32_t num_storage_textures = 0,
        uint32_t num_storage_buffers = 0,
        uint32_t num_uniform_buffers = 0
    ) -> SDL_GPUShader*
    {
        auto bytes = ReadFile(path);
        if (bytes.size() < 4)
        {
            std::cerr << "Empty shader: " << path << "\n";
            return nullptr;
        }
        if (format == SDL_GPU_SHADERFORMAT_SPIRV)
        {
            uint32_t magic = *reinterpret_cast<const uint32_t*>(bytes.data());
            if (magic != 0x07230203)
            {
                std::cerr << "Not SPIR-V: " << path << "\n";
                return nullptr;
            }
        }
        SDL_GPUShaderCreateInfo info{};
        info.stage = stage;
        info.format = format;
        info.entrypoint = "main";
        info.code = bytes.data();
        info.code_size = bytes.size();

        info.num_samplers =  num_samplers;
        info.num_storage_textures = num_storage_textures;
        info.num_storage_buffers = num_storage_buffers;
        info.num_uniform_buffers = num_uniform_buffers;

        SDL_GPUShader* shader = SDL_CreateGPUShader(window.GetGPUDevice(), &info);
        if (!shader) std::cerr << "Error: Failed to create GPU shader: " << path << ": " << SDL_GetError(); 
        return shader;
    };

    SDL_GPUShader* vertex_shader = nullptr;
    SDL_GPUShader* fragment_shader = nullptr;

    if (formats & SDL_GPU_SHADERFORMAT_SPIRV)
    {
        vertex_shader = load("shaders/build/grid_fullscreen_triangle.vert.spv", SDL_GPU_SHADERSTAGE_VERTEX, SDL_GPU_SHADERFORMAT_SPIRV, 0);
        fragment_shader = load("shaders/build/grid_sample_rgba8.frag.spv", SDL_GPU_SHADERSTAGE_FRAGMENT, SDL_GPU_SHADERFORMAT_SPIRV, 1);
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
        SDL_GPUTextureFormat swapchain_format = renderer.GetSwapchainFormat();

        SDL_GPUColorTargetBlendState blend_state{};
        blend_state.enable_blend = false;
        blend_state.enable_color_write_mask = true;
        blend_state.color_write_mask =
        (
            SDL_GPU_COLORCOMPONENT_R |
            SDL_GPU_COLORCOMPONENT_G |
            SDL_GPU_COLORCOMPONENT_B |
            SDL_GPU_COLORCOMPONENT_A
        );

        SDL_GPUColorTargetDescription color_desc{};
        color_desc.format = swapchain_format;
        color_desc.blend_state = blend_state;

        // bundle description into target info
        SDL_GPUGraphicsPipelineTargetInfo pipeline_target_info{};
        pipeline_target_info.color_target_descriptions  = &color_desc;
        pipeline_target_info.num_color_targets          = 1;
        pipeline_target_info.has_depth_stencil_target   = false;

        // set conservative rasterizer defaults
        SDL_GPURasterizerState rasterizer_state{};
        rasterizer_state.fill_mode          = SDL_GPU_FILLMODE_FILL;
        rasterizer_state.cull_mode          = SDL_GPU_CULLMODE_NONE;
        rasterizer_state.front_face         = SDL_GPU_FRONTFACE_COUNTER_CLOCKWISE;
        rasterizer_state.enable_depth_clip  = true;

        // pipeline
        SDL_GPUGraphicsPipelineCreateInfo pipeline_info{};
        pipeline_info.vertex_shader         = vertex_shader;
        pipeline_info.fragment_shader       = fragment_shader;
        pipeline_info.vertex_input_state    = vertex_input_state;
        pipeline_info.primitive_type        = SDL_GPU_PRIMITIVETYPE_TRIANGLELIST;
        pipeline_info.rasterizer_state      = rasterizer_state;
        pipeline_info.target_info           = pipeline_target_info;
   
        SDL_GPUGraphicsPipeline* grid_pipeline = SDL_CreateGPUGraphicsPipeline(window.GetGPUDevice(), &pipeline_info);
        if (!grid_pipeline)
        {
            std::cerr << "Error: Failed to create GPU graphics pipeline: " << SDL_GetError() << "\n";
            return 1;
        }


        // CA Create GPU Texture
        SDL_GPUTexture* cellular_automata_texture = nullptr;
        {
            SDL_GPUTextureCreateInfo texture_info{};
            texture_info.type                   = SDL_GPU_TEXTURETYPE_2D;
            texture_info.format                 = SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM;
            texture_info.usage                  = SDL_GPU_TEXTUREUSAGE_SAMPLER;
            texture_info.width                  = auto_sim.GetWidth();
            texture_info.height                 = auto_sim.GetHeight();
            texture_info.layer_count_or_depth   = 1;
            texture_info.num_levels             = 1;
            cellular_automata_texture           = SDL_CreateGPUTexture(window.GetGPUDevice(), &texture_info);
            if (!cellular_automata_texture)
            {
                std::cerr << "Error: Failed to create GPU texture: " << SDL_GetError() << "\n";
                return 1;   
            }
        }

        // CA Create GPU Transfer Buffer
        SDL_GPUTransferBuffer* transfer_buffer = nullptr;
        {
            SDL_GPUTransferBufferCreateInfo upload{};
            upload.usage    = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD;
            upload.size     = auto_sim.GetWidth() * auto_sim.GetHeight() * 4;
            transfer_buffer = SDL_CreateGPUTransferBuffer(window.GetGPUDevice(), &upload); 
            if (!transfer_buffer)
            {
                std::cerr << "Error: Failed to create GPU transfer buffer: " << SDL_GetError() << "\n";
                return 1;
            }
        }
        

        // for crisp scaling in imgui
        SDL_GPUSampler* nearest_sampler = nullptr;
        {
            SDL_GPUSamplerCreateInfo sampler_info{};
            sampler_info.min_filter     = SDL_GPU_FILTER_NEAREST;
            sampler_info.mag_filter     = SDL_GPU_FILTER_NEAREST;
            sampler_info.mipmap_mode    = SDL_GPU_SAMPLERMIPMAPMODE_NEAREST;
            sampler_info.address_mode_u = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE; 
            sampler_info.address_mode_v = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE;
            sampler_info.address_mode_w = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE;
            nearest_sampler = SDL_CreateGPUSampler(window.GetGPUDevice(), &sampler_info);
            if (!nearest_sampler)
            {
                std::cerr << "Error: Failed to create GPU sampler: " << SDL_GetError() << "\n";
                return 1;   
            }
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
            auto_sim.Step();

            imgui_manager.BeginFrame();
            imgui_manager.Build();
            imgui_manager.EndFrame();
            auto* draw_data = imgui_manager.GetDrawData();
            const bool is_minimized = (draw_data->DisplaySize.x <= 0.0f || draw_data->DisplaySize.y <= 0.0f);
    
            
            const auto& grid_data = *auto_sim.GetGrid();
            int W = auto_sim.GetWidth();
            int H = auto_sim.GetHeight();
            std::vector<uint32_t> pixels(W * H);
           
            auto MapColor = [](const Cell& cell)->uint32_t
            {
                math::RGBColor color = math::HSVtoRGB(cell.color);
                uint8_t r = uint8_t(color.r * 255);
                uint8_t g = uint8_t(color.g * 255);
                uint8_t b = uint8_t(color.b * 255);
                uint8_t a = 255;
                return (uint32_t(r)) | (uint32_t(g)<<8) | (uint32_t(b)<<16) | (uint32_t(a)<<24);
            };

            const size_t n = std::min(pixels.size(), grid_data.size());
            for (size_t i = 0; i < n; ++i)
            {
                bool on = (grid_data[i].state != 0);
                //pixels[i] = on ? 0xFFFF00FFu : 0xFF505050u;
                pixels[i] = MapColor(grid_data[i]);
           
            }
            
            renderer.AcquireCommandBuffer();
            SDL_GPUCommandBuffer* command_buffer = renderer.GetCommandBuffer();

            // upload pixels to ca texture in a COPY PASS
            void* dst = SDL_MapGPUTransferBuffer(window.GetGPUDevice(), transfer_buffer, true);
            if (!dst)
            {
                std::cout << "Error: Map GPU transfer buffer failed: " << SDL_GetError() << "\n"; 
                return 1;
            }

            std::memcpy(dst, pixels.data(), pixels.size() * sizeof(uint32_t));
            SDL_UnmapGPUTransferBuffer(window.GetGPUDevice(), transfer_buffer);


            SDL_GPUTextureTransferInfo tti{};
            tti.transfer_buffer = transfer_buffer;
            tti.offset = 0;
            tti.pixels_per_row = W;
            tti.rows_per_layer = H;

            SDL_GPUTextureRegion dst_region{};
            dst_region.texture = cellular_automata_texture;
            dst_region.mip_level = 0;
            dst_region.layer = 0;
            dst_region.x = 0; dst_region.y = 0; dst_region.z = 0;
            dst_region.w = W; dst_region.h = H; dst_region.d = 1;
            
            SDL_GPUCopyPass* copy_pass = SDL_BeginGPUCopyPass(command_buffer);
            if (!copy_pass)
            {
                std::cerr << "Error: Failed to begin Copy Pass: " << SDL_GetError() << "\n";
                return 1;
            }
            SDL_UploadToGPUTexture(copy_pass, &tti, &dst_region, true);
            SDL_EndGPUCopyPass(copy_pass);


            imgui_manager.Prepare(renderer.GetCommandBuffer());
            renderer.AcquireSwapchainTexture();
            auto* swapchain_texture = renderer.GetSwapchainTexture();
            if (swapchain_texture && !is_minimized)
            {
                renderer.SetColorTargetInfo(math::RGBAColor(0.0f, 0.0f, 0.0f, 1.0f));
                renderer.BeginRenderPass();
                SDL_GPURenderPass* render_pass = renderer.GetRenderPass();
                
                // bind pipeline, verter buffer, and the ca texture + sampler at FS slot 0
                SDL_BindGPUGraphicsPipeline(render_pass, grid_pipeline);

                SDL_GPUTextureSamplerBinding texture_sampling{};
                texture_sampling.texture = cellular_automata_texture;
                texture_sampling.sampler = nearest_sampler;

                SDL_BindGPUFragmentSamplers(render_pass, 0, &texture_sampling, 1);

                SDL_DrawGPUPrimitives(render_pass, 3, 1, 0, 0);
                
                imgui_manager.Draw(render_pass, renderer.GetCommandBuffer());
                
                renderer.EndRenderPass();
            }
            renderer.SubmitCommandBuffer();
        }
        SDL_ReleaseGPUTexture(window.GetGPUDevice(), cellular_automata_texture);
        SDL_ReleaseGPUSampler(window.GetGPUDevice(), nearest_sampler);
        SDL_ReleaseGPUTransferBuffer(window.GetGPUDevice(), transfer_buffer);
        SDL_ReleaseGPUGraphicsPipeline(window.GetGPUDevice(), grid_pipeline);
        SDL_ReleaseGPUShader(window.GetGPUDevice(), fragment_shader);
        SDL_ReleaseGPUShader(window.GetGPUDevice(), vertex_shader);
    }
    return 0;
}

static std::vector<uint8_t> ReadFile(const char* path)
{
    std::FILE* file = std::fopen(path, "rb");
    if (!file)
    {
        std::cerr << "Warning: File not found: " << path << "\n";
        return {};
    }

    std::fseek(file, 0, SEEK_END);
    long size = std::ftell(file);
    std::fseek(file, 0, SEEK_SET);

    std::vector<uint8_t> buffer((size_t)size);
    if (size > 0)
    {
        std::fread(buffer.data(), 1, (size_t)size, file);
    }
    
    std::fclose(file);
    return buffer;
}