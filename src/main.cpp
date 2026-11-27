#include <SDL3/SDL.h>
#include <array>
#include <vector>
#include "automata_gpu.hpp"
#include "automata_simulator.hpp"
#include "config.hpp"
#include "event_handler.hpp"
#include "imgui_manager.hpp"
#include "renderer.hpp"
#include "window.hpp"

namespace
{
std::vector<uint8_t> MakeSeed(const SimulationControls& controls)
{
    AutomataSimulator simulator(config::GRID_WIDTH, config::GRID_HEIGHT);
    uset birth, survival;
    for (int count = 0; count <= SimulationControls::MAX_NEIGHBORS; ++count)
    {
        if (controls.birth_mask & (1u << count))
            birth.insert(count);
        if (controls.survival_mask & (1u << count))
            survival.insert(count);
    }
    simulator.SetRule(birth, survival, SimulationControls::NEIGHBOR_RANGE, config::MAX_STATES);
    simulator.SetDecay(0.0f);

    const std::array<math::ivec2, 12> positions{{
        {1, 3}, {2, 3}, {3, 3}, {12, 24}, {14, 24}, {15, 25},
        {15, 26}, {15, 27}, {12, 27}, {13, 28}, {14, 28}, {15, 28}
    }};
    auto& grid = *simulator.GetGrid();
    for (const auto position : positions)
    {
        if (!simulator.IsOutOfBounds(position))
            grid[simulator.GetIndex(position)].state = 1;
    }

    simulator.Step();
    simulator.Step();

    std::vector<uint8_t> seed;
    seed.reserve(grid.size());
    for (const auto& cell : grid)
        seed.push_back(cell.state != 0 ? 255 : 0);
    return seed;
}

int ReportError(const char* action)
{
    SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "%s: %s", action, SDL_GetError());
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
                             config::MAX_STATES, MakeSeed(controls)))
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
