#include <SDL3/SDL.h>
#include <array>
#include <vector>
#include "automata_gpu.hpp"
#include "config.hpp"
#include "event_handler.hpp"
#include "imgui_manager.hpp"
#include "logger.hpp"
#include "renderer.hpp"
#include "window.hpp"

namespace
{
std::vector<GpuCell> MakeSeed()
{
    struct SeedCell
    {
        int x;
        int y;
        uint16_t hue;
    };

    constexpr uint16_t violet = 52428; // 0.8
    constexpr uint16_t cyan = 32768;   // 0.5
    constexpr uint16_t green = 16384;  // 0.25
    constexpr std::array<SeedCell, 12> positions{{
        {1, 3, violet}, {2, 3, cyan}, {3, 3, green},
        {12, 24, green}, {14, 24, green}, {15, 25, green},
        {15, 26, green}, {12, 27, green}, {15, 27, green},
        {13, 28, green}, {14, 28, green}, {15, 28, green}
    }};

    std::vector<GpuCell> seed(static_cast<size_t>(config::GRID_WIDTH) * config::GRID_HEIGHT);
    for (const auto& cell : positions)
    {
        if (cell.x < config::GRID_WIDTH && cell.y < config::GRID_HEIGHT)
            seed[cell.x + cell.y * config::GRID_WIDTH] = {cell.hue, 26214, UINT16_MAX, UINT16_MAX};
    }
    return seed;
}

int ReportError(const char* action)
{
    logger::Error("{}: {}", action, SDL_GetError());
    return 1;
}
}

int main()
{
    Window window(config::WINDOW_WIDTH, config::WINDOW_HEIGHT, config::WINDOW_TITLE);
    if (!window.SetupWindow())
        return 1;

    SimulationControls controls;
    Renderer renderer(&window);
    AutomataGpu automata(window.GetGPUDevice());
    if (!automata.Initialize(renderer.GetSwapchainFormat(), config::GRID_WIDTH, config::GRID_HEIGHT,
                             config::MAX_STATES, MakeSeed()))
        return ReportError("Initializing GPU simulation");

    ImGuiManager imgui(&window);
    imgui.CreateAndSetupContext();
    imgui.SetupScaling();
    if (!imgui.SetupBackends())
        return ReportError("Initializing ImGui");

    EventHandler events(&window, &imgui);
    SDL_ShowWindow(window.GetSDLWindow());
    window.StartRun();
    while (window.IsRunning())
    {
        events.ProcessEvents();
        if (!window.IsRunning())
            break;
        if (SDL_GetWindowFlags(window.GetSDLWindow()) & SDL_WINDOW_MINIMIZED)
        {
            SDL_Delay(10);
            continue;
        }

        imgui.BeginFrame();
        const bool single_step = imgui.Build(controls);
        imgui.EndFrame();

        renderer.AcquireCommandBuffer();
        auto* commands = renderer.GetCommandBuffer();
        if (!commands)
            return ReportError("Acquiring GPU command buffer");
        if (!renderer.AcquireSwapchainTexture())
        {
            SDL_CancelGPUCommandBuffer(commands);
            return ReportError("Acquiring swapchain texture");
        }

        if (renderer.GetSwapchainTexture())
        {
            if (!controls.paused || single_step)
                automata.Step(commands, controls);
            imgui.Prepare(commands);
            renderer.SetColorTargetInfo(math::RGBAColor(0.0f, 0.0f, 0.0f, 1.0f));
            renderer.BeginRenderPass();
            auto* pass = renderer.GetRenderPass();
            automata.Draw(pass);
            imgui.Draw(pass, commands);
            renderer.EndRenderPass();
        }

        if (!renderer.SubmitCommandBuffer())
            return ReportError("Submitting GPU frame");
    }
}
