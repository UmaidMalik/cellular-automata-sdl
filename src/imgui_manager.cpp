#include "config.hpp"
#include "imgui_manager.hpp"
#include "imgui.h"
#include "imgui_impl_sdl3.h"
#include "imgui_impl_sdlgpu3.h"
#include "window.hpp"
#include <iostream>

ImGuiManager::ImGuiManager(Window* window) 
    : m_window(window)
{
    if (!m_window || !m_window->GetSDLWindow() || !m_window->GetGPUDevice())
    {
        throw std::runtime_error("ImGuiManager: window/device not ready");
    }
}

ImGuiManager::~ImGuiManager()
{
    if (!m_context)
    {
        return;
    }

    if (m_window && m_window->GetSDLWindow())
    {
        SDL_WaitForGPUIdle(m_window->GetGPUDevice());
    }

    ImGui::SetCurrentContext(m_context);
    if (m_backends_ready)
        ShutdownBackends();
    ImGui::DestroyContext(m_context);

    m_context = nullptr;
    m_imgui_io = nullptr;
    m_style = nullptr;
    m_window = nullptr;

    std::cout << "~ImGuiManager() destructor called and exited\n";
}

void ImGuiManager::CreateAndSetupContext()
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

ImGuiIO* ImGuiManager::GetIO()
{
    return m_imgui_io;
}

void ImGuiManager::SetupScaling()
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

bool ImGuiManager::SetupBackends()
{
    if (!m_window)
    {
        std::cerr << "Error: ImGuiManager::SetupBackends(): window is null\n";
        return false;
    }

    if (!ImGui_ImplSDL3_InitForSDLGPU(m_window->GetSDLWindow()))
    {
        std::cerr << "Error: ImGui_ImplSDL3_InitForSDLGPU() failed\n";
        return false;
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
        return false;
    }
    m_backends_ready = true;
    return true;
}

void ImGuiManager::LoadFonts(const char* font_path, float font_size)
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

void ImGuiManager::ProcessEvent(SDL_Event* event)
{
    ImGui_ImplSDL3_ProcessEvent(event);
}

void ImGuiManager::ShutdownBackends()
{
    ImGui_ImplSDLGPU3_Shutdown();
    ImGui_ImplSDL3_Shutdown();
}

void ImGuiManager::BeginFrame()
{
    ImGui_ImplSDLGPU3_NewFrame();
    ImGui_ImplSDL3_NewFrame();
    ImGui::NewFrame();
}

namespace
{
void EditRule(const char* label, unsigned int& mask)
{
    ImGui::TextUnformatted(label);
    ImGui::PushID(label);
    for (int count = 0; count <= SimulationControls::MAX_NEIGHBORS; ++count)
    {
        if (count > 0)
            ImGui::SameLine();
        const char text[] = {static_cast<char>('0' + count), '\0'};
        ImGui::CheckboxFlags(text, &mask, 1u << count);
    }
    ImGui::PopID();
}
}

bool ImGuiManager::Build(SimulationControls& controls)
{
    bool step = false;
    if (ImGui::Begin("Simulation", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
    {
        ImGui::Checkbox("Paused", &controls.paused);
        ImGui::SameLine();
        ImGui::BeginDisabled(!controls.paused);
        step = ImGui::Button("Step");
        ImGui::EndDisabled();

        ImGui::Checkbox("Wrap edges", &controls.wrap_edges);
        bool fullscreen = (SDL_GetWindowFlags(m_window->GetSDLWindow()) & SDL_WINDOW_FULLSCREEN) != 0;
        if (ImGui::Checkbox("Fullscreen", &fullscreen))
            m_window->SetFullscreen(fullscreen);

        EditRule("Birth neighbors", controls.birth_mask);
        EditRule("Survival neighbors", controls.survival_mask);
        ImGui::Text("%.1f FPS", m_imgui_io->Framerate);
    }
    ImGui::End();
    return step;
}

void ImGuiManager::Prepare(SDL_GPUCommandBuffer* command_buffer)
{
    ImGui_ImplSDLGPU3_PrepareDrawData(m_draw_data, command_buffer);

}

void ImGuiManager::Draw(SDL_GPURenderPass* render_pass, SDL_GPUCommandBuffer* command_buffer)
{
    ImGui_ImplSDLGPU3_RenderDrawData(m_draw_data, command_buffer, render_pass);
}

void ImGuiManager::EndFrame()
{
    ImGui::Render();
    m_draw_data = ImGui::GetDrawData();
}

ImDrawData* ImGuiManager::GetDrawData()
{
    return m_draw_data;
}