#pragma once

#include "imgui.h"
#include "imgui_impl_sdl3.h"
#include "imgui_impl_sdlgpu3.h"
#include "window.hpp"
#include <iostream>

struct SimulationControls;

class ImGuiManager
{
    private:
        ImGuiContext* m_context = nullptr;
        ImGuiIO* m_imgui_io = nullptr;
        ImGuiStyle* m_style = nullptr;
        Window* m_window = nullptr;
        ImDrawData* m_draw_data = nullptr;
        ImGui_ImplSDLGPU3_InitInfo m_init_info{};
        bool m_backends_ready = false;
    private:
        void ShutdownBackends();
    public:
        ImGuiManager(Window* window);
        ~ImGuiManager();

        // non-copyable / non-movable
        //ImGuiManager(const ImGuiManager&) = delete;
        //ImGuiManager& operator=(const ImGuiManager&) = delete;
        //ImGuiManager(ImGuiManager&&) = delete;
        //ImGuiManager& operator=(ImGuiManager&&) = delete;

        void CreateAndSetupContext();
        ImGuiIO* GetIO();
        void SetupScaling();
        bool SetupBackends();
        void LoadFonts(const char* font_path, float font_size);
        void ProcessEvent(SDL_Event* event);
        void BeginFrame();
        bool Build(SimulationControls& controls);
        void Prepare(SDL_GPUCommandBuffer* command);
        void Draw(SDL_GPURenderPass* render_pass, SDL_GPUCommandBuffer* command_buffer);
        void EndFrame();
        ImDrawData* GetDrawData();
};