#include <SDL3/SDL.h>
#include <algorithm>
#include <cmath>
#include <optional>
#include <vector>
#include "automata_gpu.hpp"
#include "config.hpp"
#include "event_handler.hpp"
#include "grid_view.hpp"
#include "imgui_manager.hpp"
#include "logger.hpp"
#include "renderer.hpp"
#include "simulation_clock.hpp"
#include "window.hpp"

namespace
{
void AddStroke(std::vector<SDL_Point>& cells, SDL_Point start, SDL_Point end)
{
    const int dx = std::abs(end.x - start.x);
    const int dy = -std::abs(end.y - start.y);
    const int step_x = start.x < end.x ? 1 : -1;
    const int step_y = start.y < end.y ? 1 : -1;
    int error = dx + dy;
    for (;;)
    {
        cells.push_back(start);
        if (start.x == end.x && start.y == end.y)
            break;
        const int twice_error = 2 * error;
        if (twice_error >= dy)
        {
            error += dy;
            start.x += step_x;
        }
        if (twice_error <= dx)
        {
            error += dx;
            start.y += step_y;
        }
    }
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
    GridView view;
    view.SetZoom(GridView::DEFAULT_ZOOM, 0.5f, 0.5f);
    std::optional<SDL_Point> previous_cell;
    std::vector<SDL_Point> stroke;
    float paint_hue = controls.brush_hue;
    bool stroke_started = false;
    Uint32 stroke_seed = 0;
    float brush_scroll = 0.0f;
    float brush_screen_scale = controls.brush_size * view.zoom;
    bool reset_pending = false;
    bool panning = false;
    Renderer renderer(&window);
    AutomataGpu automata(window.GetGPUDevice());
    if (!automata.Initialize(renderer.GetSwapchainFormat(), config::GRID_WIDTH, config::GRID_HEIGHT))
        return ReportError("Initializing GPU simulation");

    ImGuiManager imgui(&window);
    imgui.CreateAndSetupContext();
    imgui.SetupScaling();
    if (!imgui.SetupBackends())
        return ReportError("Initializing ImGui");

    EventHandler events(&window, &imgui);
    SDL_ShowWindow(window.GetSDLWindow());
    window.StartRun();
    SimulationClock clock;
    Uint64 previous_frame = SDL_GetTicksNS();
    while (window.IsRunning())
    {
        const Uint64 now = SDL_GetTicksNS();
        const double elapsed_seconds = static_cast<double>(now - previous_frame) / SDL_NS_PER_SECOND;
        previous_frame = now;
        events.ProcessEvents();
        if (!window.IsRunning())
            break;
        if (SDL_GetWindowFlags(window.GetSDLWindow()) & SDL_WINDOW_MINIMIZED)
        {
            previous_cell.reset();
            panning = false;
            stroke_started = false;
            clock.Reset();
            SDL_Delay(10);
            continue;
        }

        imgui.BeginFrame();
        const float previous_zoom = view.zoom;
        const int previous_brush_size = controls.brush_size;
        const bool was_synced = controls.sync_brush_with_zoom;
        float zoom = view.zoom;
        const auto action = imgui.Build(controls, zoom);
        reset_pending = reset_pending || action == SimulationAction::Reset;
        if (zoom != view.zoom)
        {
            view.SetZoom(zoom, 0.5f, 0.5f);
            previous_cell.reset();
        }

        const auto& io = *imgui.GetIO();
        const ImVec2 mouse = io.MousePos;
        const ImVec2 screen = io.DisplaySize;
        stroke.clear();
        if (!ImGui::IsMouseDown(ImGuiMouseButton_Left) || ImGui::IsMouseClicked(ImGuiMouseButton_Left))
            stroke_started = false;

        const bool over_grid = !io.WantCaptureMouse && !ImGui::IsWindowHovered(ImGuiHoveredFlags_AnyWindow) &&
            mouse.x >= 0.0f && mouse.y >= 0.0f &&
            mouse.x < screen.x && mouse.y < screen.y;

        if (!ImGui::IsMouseDown(ImGuiMouseButton_Right))
            panning = false;
        if (ImGui::IsMouseClicked(ImGuiMouseButton_Right))
            panning = over_grid;
        else if (panning && screen.x > 0.0f && screen.y > 0.0f)
            view.Pan(io.MouseDelta.x / screen.x, io.MouseDelta.y / screen.y);

        const bool can_edit_grid = over_grid && !panning;
        if (can_edit_grid)
        {
            if (io.KeyShift)
            {
                brush_scroll += io.MouseWheel;
                const int size_change = static_cast<int>(brush_scroll);
                controls.brush_size = std::clamp(
                    controls.brush_size + size_change, 1, SimulationControls::MAX_BRUSH_SIZE);
                brush_scroll -= size_change;
            }
            else if (io.MouseWheel != 0.0f)
            {
                view.SetZoom(view.zoom * std::pow(1.2f, io.MouseWheel),
                             mouse.x / screen.x, mouse.y / screen.y);
                previous_cell.reset();
            }
        }

        if (!controls.sync_brush_with_zoom || !was_synced || controls.brush_size != previous_brush_size)
        {
            brush_screen_scale = controls.brush_size * view.zoom;
        }
        else if (view.zoom != previous_zoom)
        {
            // Keep the reference even when rounding or clamping the grid diameter.
            const int size = static_cast<int>(std::lround(brush_screen_scale / view.zoom));
            controls.brush_size = std::clamp(size, 1, SimulationControls::MAX_BRUSH_SIZE);
        }

        if (can_edit_grid)
        {
            if (ImGui::IsMouseClicked(ImGuiMouseButton_Left))
                previous_cell.reset();
            if (ImGui::IsMouseDown(ImGuiMouseButton_Left))
            {
                if (!stroke_started)
                {
                    if (controls.random_brush_hue)
                    {
                        // Stay at least one eighth of the hue wheel away from the last stroke.
                        paint_hue = std::fmod(paint_hue + 0.125f + 0.75f * SDL_randf(), 1.0f);
                    }
                    else
                    {
                        paint_hue = controls.brush_hue;
                    }
                    stroke_seed = SDL_rand_bits();
                    stroke_started = true;
                }
                const SDL_Point cell = view.CellAt(mouse.x / screen.x, mouse.y / screen.y,
                                                   config::GRID_WIDTH, config::GRID_HEIGHT);
                AddStroke(stroke, previous_cell.value_or(cell), cell);
                previous_cell = cell;
            }
        }
        if (stroke.empty())
            previous_cell.reset();
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

        const bool can_draw = renderer.GetSwapchainTexture() != nullptr;
        const bool paused = controls.paused || (controls.pause_on_draw && !stroke.empty());
        if (paused || reset_pending || !can_draw)
            clock.Reset();

        if (can_draw)
        {
            if (reset_pending)
            {
                automata.Clear(commands);
                controls.paused = true;
                reset_pending = false;
                stroke.clear();
                previous_cell.reset();
            }
            else
            {
                int steps = 0;
                if (action == SimulationAction::Step)
                    steps = 1;
                else if (!paused)
                    steps = clock.Advance(elapsed_seconds, controls.generations_per_second);

                for (int step = 0; step < steps; ++step)
                    automata.Step(commands, controls);
            }
            if (!automata.Paint(commands, stroke, paint_hue, controls.brush_size, stroke_seed))
            {
                // A command buffer with an acquired swapchain must be submitted.
                renderer.SubmitCommandBuffer();
                return ReportError("Uploading painted cells");
            }
            imgui.Prepare(commands);
            renderer.SetColorTargetInfo(math::RGBAColor(0.0f, 0.0f, 0.0f, 1.0f));
            renderer.BeginRenderPass();
            auto* pass = renderer.GetRenderPass();
            automata.Draw(commands, pass, view);
            imgui.Draw(pass, commands);
            renderer.EndRenderPass();
        }

        if (!renderer.SubmitCommandBuffer())
            return ReportError("Submitting GPU frame");
    }
}
