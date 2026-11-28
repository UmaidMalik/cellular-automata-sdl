#pragma once

#include "imgui.h"
#include "imgui_impl_sdl3.h"
#ifndef __EMSCRIPTEN__
#include "imgui_impl_sdlgpu3.h"
#endif
#include "window.hpp"
#include <iostream>

struct SimulationControls;

enum class SimulationAction
{
    None,
    Step,
    Reset,
    ResizeGrid
};

class ImGuiManager
{
    private:
        ImGuiContext* m_context = nullptr;
        ImGuiIO* m_imgui_io = nullptr;
        ImGuiStyle* m_style = nullptr;
        Window* m_window = nullptr;
        ImDrawData* m_draw_data = nullptr;
#ifndef __EMSCRIPTEN__
        ImGui_ImplSDLGPU3_InitInfo m_init_info{};
#endif
        bool m_backends_ready = false;
        bool m_ui_hidden = false;
        bool m_hide_shortcut_pending = false;
        bool m_pause_shortcut_pending = false;
        bool m_mobile_settings_open = false;
        bool m_mobile_scroll_touch = false;
        Uint64 m_hide_until = 0;
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
        SimulationAction Build(SimulationControls& controls, float& zoom, int& aspect_index,
                               SDL_Point grid, int max_grid_size, bool resize_failed, bool protection_activated);
#ifdef __EMSCRIPTEN__
        void Draw();
#else
        void Prepare(SDL_GPUCommandBuffer* command);
        void Draw(SDL_GPURenderPass* render_pass, SDL_GPUCommandBuffer* command_buffer);
#endif
        void EndFrame();
        ImDrawData* GetDrawData();
};
