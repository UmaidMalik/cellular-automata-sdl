#pragma once

#include "imgui.h"
#include "imgui_impl_sdl3.h"
#include "imgui_impl_sdlgpu3.h"
#include "window.hpp"
#include <iostream>

struct UIControl
{
    bool show_demo_window = false;
    bool set_fullscreen = false;
};

class ImGuiManager
{
    private:
        ImGuiContext* m_context = nullptr;
        ImGuiIO* m_imgui_io = nullptr;
        ImGuiStyle* m_style = nullptr;
        Window* m_window = nullptr;
        ImDrawData* m_draw_data = nullptr;
        ImGui_ImplSDLGPU3_InitInfo m_init_info{};
        UIControl m_ui_control;
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
        void SetupBackends();
        void LoadFonts(const char* font_path, float font_size);
        void ProcessEvent(SDL_Event* event);
        void BeginFrame();
        void Build();
        void Prepare(SDL_GPUCommandBuffer* command);
        void Draw(SDL_GPURenderPass* render_pass, SDL_GPUCommandBuffer* command_buffer);
        void ShowDemoWindow();
        void EndFrame();
        ImDrawData* GetDrawData();
};