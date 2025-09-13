#pragma once

#include "imgui.h"
#include "imgui_impl_sdl3.h"
#include "imgui_impl_sdlgpu3.h"
#include "window.hpp"
#include <iostream>

class ImGuiManager
{
    private:
        ImGuiIO* m_imgui_io;
        ImGuiStyle* m_style;
        Window* m_window;
        ImGui_ImplSDLGPU3_InitInfo m_init_info;
    public:
        ImGuiManager(Window* window);
        ~ImGuiManager();
        void CreateAndSetupContext();
        ImGuiIO* GetIO();
        void SetupScaling();
        void SetupBackends();
};

ImGuiManager::ImGuiManager(Window* window) 
    : m_imgui_io(nullptr), m_style(nullptr), m_window(window)
{
    m_init_info = {};
}

ImGuiManager::~ImGuiManager()
{
    ImGui::DestroyContext();
    m_imgui_io = nullptr;
}

void ImGuiManager::CreateAndSetupContext()
{
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    m_imgui_io = &ImGui::GetIO(); (void)m_imgui_io;
    m_imgui_io->ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
    m_imgui_io->ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls

    // Setup style
    ImGui::StyleColorsDark();
    m_style = &ImGui::GetStyle();
}

ImGuiIO* ImGuiManager::GetIO()
{
    return m_imgui_io;
}

void ImGuiManager::SetupScaling()
{
    if (!m_window)
        std::cerr << "Error: ImGuiManager::SetupScaling(): window is null\n";
        return;
    if (!m_style || !m_imgui_io)
        std::cerr << "Error: ImGuiManager::SetupScaling(): ImGui context not initialized\n";
        return;
    m_style->ScaleAllSizes(m_window->GetMainScale()); // Bake a fixed style scale. (until we have a solution for dynamic style scaling, changing this requires resetting Style + calling this again)
    m_imgui_io->FontGlobalScale = m_window->GetMainScale(); // Set initial font scale. (using io.ConfigDpiScaleFonts=true makes this unnecessary. We leave both here for documentation purpose)
}

void ImGuiManager::SetupBackends()
{
    if (!m_window)
    {
        std::cerr << "Error: ImGuiManager::SetupBackends(): window is null\n";
        return;
    }
    
    ImGui_ImplSDL3_InitForSDLGPU(m_window->GetSDLWindow());
    m_init_info.Device = m_window->GetGPUDevice();
    m_init_info.ColorTargetFormat = SDL_GetGPUSwapchainTextureFormat
    (
        m_window->GetGPUDevice(),
        m_window->GetSDLWindow()
    );
    m_init_info.MSAASamples = SDL_GPU_SAMPLECOUNT_1;                      // Only used in multi-viewports mode.
    m_init_info.SwapchainComposition = SDL_GPU_SWAPCHAINCOMPOSITION_SDR;  // Only used in multi-viewports mode.
    m_init_info.PresentMode = SDL_GPU_PRESENTMODE_VSYNC;
    ImGui_ImplSDLGPU3_Init(&m_init_info);
}