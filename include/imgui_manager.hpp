#pragma once

#include "imgui.h"
#include "imgui_impl_sdl3.h"
#include "imgui_impl_sdlgpu3.h"
#include "window.hpp"
#include <iostream>

class ImGuiManager
{
    private:
        ImGuiContext* m_context = nullptr;
        ImGuiIO* m_imgui_io = nullptr;
        ImGuiStyle* m_style = nullptr;
        Window* m_window = nullptr;
        ImDrawData* m_draw_data = nullptr;
        ImGui_ImplSDLGPU3_InitInfo m_init_info{};
    private:
        void ShutdownBackends();
    public:
        // NOTE: For learning ImGui, will delete when not needed
        bool m_show_demo_window = false;
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
        void StartNewFrame();  
        void ShowDemoWindow();
        void Render();
        ImDrawData* GetDrawData();
};

inline ImGuiManager::ImGuiManager(Window* window) 
    : m_window(window)
{
    if (!m_window || !m_window->GetSDLWindow() || !m_window->GetGPUDevice())
    {
        throw std::runtime_error("ImGuiManager: window/device not ready");
    }
}

inline ImGuiManager::~ImGuiManager()
{
    if (!m_context)
    {
        return;
    }

    if (m_window && m_window->GetSDLWindow())
    {
        SDL_WaitForGPUIdle(m_window->GetGPUDevice());
    }

    ShutdownBackends();
    ImGui::SetCurrentContext(m_context);
    ImGui::DestroyContext(m_context);

    m_context = nullptr;
    m_imgui_io = nullptr;
    m_style = nullptr;
    m_window = nullptr;

    std::cout << "~ImGuiManager() destructor called and exited\n";
}

inline void ImGuiManager::CreateAndSetupContext()
{
    IMGUI_CHECKVERSION();
    m_context = ImGui::CreateContext();
    if (!m_context)
    {
        throw std::runtime_error("ImGuiManager: failed to create ImGui context");
    }

    m_imgui_io = &ImGui::GetIO(); (void)m_imgui_io;
    m_imgui_io->ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
    m_imgui_io->ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls

    // Setup style
    ImGui::StyleColorsDark();
    m_style = &ImGui::GetStyle();
}

inline ImGuiIO* ImGuiManager::GetIO()
{
    return m_imgui_io;
}

inline void ImGuiManager::SetupScaling()
{
    if (!m_window)
    {
        std::cerr << "Error: ImGuiManager::SetupScaling(): window is null\n";
        return;
    }

    if (!m_style || !m_imgui_io)
    {
        std::cerr << "Error: ImGuiManager::SetupScaling(): ImGui context not initialized\n";
        return;
    }

    const float scale = m_window->GetMainScale();
    m_style->ScaleAllSizes(scale); // Bake a fixed style scale. (until we have a solution for dynamic style scaling, changing this requires resetting Style + calling this again)
    m_imgui_io->FontGlobalScale = scale; // Set initial font scale. (using io.ConfigDpiScaleFonts=true makes this unnecessary. We leave both here for documentation purpose)
}

inline void ImGuiManager::SetupBackends()
{
    if (!m_window)
    {
        std::cerr << "Error: ImGuiManager::SetupBackends(): window is null\n";
        return;
    }

    if (!ImGui_ImplSDL3_InitForSDLGPU(m_window->GetSDLWindow()))
    {
        std::cerr << "Error: ImGui_ImplSDL3_InitForSDLGPU() failed\n";
        return;
    }

    m_init_info = {};
    m_init_info.Device = m_window->GetGPUDevice();
    m_init_info.ColorTargetFormat = SDL_GetGPUSwapchainTextureFormat
    (
        m_window->GetGPUDevice(),
        m_window->GetSDLWindow()
    );
    m_init_info.MSAASamples = SDL_GPU_SAMPLECOUNT_1;                      // Only used in multi-viewports mode.
    m_init_info.SwapchainComposition = SDL_GPU_SWAPCHAINCOMPOSITION_SDR;  // Only used in multi-viewports mode.
    m_init_info.PresentMode = SDL_GPU_PRESENTMODE_VSYNC;
    
    if (!ImGui_ImplSDLGPU3_Init(&m_init_info))
    {
        std::cerr << "Error: ImGui_ImplSDLGPU3_Init() failed\n";
        ImGui_ImplSDL3_Shutdown();
        return;
    }
}

inline void ImGuiManager::LoadFonts(const char* font_path, float font_size)
{
    // Load Fonts
    // - If no fonts are loaded, dear imgui will use the default font. You can also load multiple fonts and use ImGui::PushFont()/PopFont() to select them.
    // - AddFontFromFileTTF() will return the ImFont* so you can store it if you need to select the font among multiple.
    // - If the file cannot be loaded, the function will return a nullptr. Please handle those errors in your application (e.g. use an assertion, or display an error and quit).
    // - Use '#define IMGUI_ENABLE_FREETYPE' in your imconfig file to use Freetype for higher quality font rendering.
    // - Read 'docs/FONTS.md' for more instructions and details. If you like the default font but want it to scale better, consider using the 'ProggyVector' from the same author!
    // - Remember that in C/C++ if you want to include a backslash \ in a string literal you need to write a double backslash \\ !
    //style.FontSizeBase = 20.0f;
    //io.Fonts->AddFontDefault();
    //io.Fonts->AddFontFromFileTTF("c:\\Windows\\Fonts\\segoeui.ttf");
    //io.Fonts->AddFontFromFileTTF("../../misc/fonts/DroidSans.ttf");
    //io.Fonts->AddFontFromFileTTF("../../misc/fonts/Roboto-Medium.ttf");
    //io.Fonts->AddFontFromFileTTF("../../misc/fonts/Cousine-Regular.ttf");
    //ImFont* font = io.Fonts->AddFontFromFileTTF("c:\\Windows\\Fonts\\ArialUni.ttf");
    //IM_ASSERT(font != nullptr);

}

inline void ImGuiManager::ProcessEvent(SDL_Event* event)
{
    ImGui_ImplSDL3_ProcessEvent(event);
}

inline void ImGuiManager::ShutdownBackends()
{
    ImGui_ImplSDLGPU3_Shutdown();
    ImGui_ImplSDL3_Shutdown();
}

inline void ImGuiManager::StartNewFrame()
{
    ImGui_ImplSDLGPU3_NewFrame();
    ImGui_ImplSDL3_NewFrame();
    ImGui::NewFrame();
}

// NOTE: For learning ImGui, will delete when not needed
inline void ImGuiManager::ShowDemoWindow()
{
    ImGui::ShowDemoWindow(&m_show_demo_window);
}

inline void ImGuiManager::Render()
{
    ImGui::Render();
    m_draw_data = ImGui::GetDrawData();
}

inline ImDrawData* ImGuiManager::GetDrawData()
{
    return m_draw_data;
}