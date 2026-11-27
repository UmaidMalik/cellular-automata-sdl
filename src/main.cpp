#include <SDL3/SDL.h>
#include <algorithm>
#include <cmath>
#include <optional>
#include <vector>
#ifdef __EMSCRIPTEN__
#include <SDL3/SDL_main.h>
#include <emscripten/emscripten.h>
#include "web_automata.hpp"
#else
#include "automata_gpu.hpp"
#include "renderer.hpp"
#endif
#include <memory>
#include "config.hpp"
#include "event_handler.hpp"
#include "grid_view.hpp"
#include "imgui_manager.hpp"
#include "logger.hpp"
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

SDL_AppResult ReportError(const char* action)
{
    logger::Error("{}: {}", action, SDL_GetError());
    return SDL_APP_FAILURE;
}

class Application
{
public:
    SDL_AppResult Initialize();
    SDL_AppResult Frame();

private:
    Window window{config::WINDOW_WIDTH, config::WINDOW_HEIGHT, config::WINDOW_TITLE};
#ifdef __EMSCRIPTEN__
    WebAutomata automata;
#else
    Renderer renderer{&window};
    // The GPU device is created during Initialize.
    std::unique_ptr<AutomataGpu> automata_storage;
#endif
    ImGuiManager imgui{&window};
    EventHandler events{&window, &imgui};
    SimulationControls controls;
    GridView view;
    SimulationClock clock;
    Uint64 previous_frame = 0;
    std::optional<SDL_Point> previous_cell;
    std::vector<SDL_Point> stroke;
    float paint_hue = controls.brush_hue;
    bool stroke_started = false;
    Uint32 stroke_seed = 0;
    float brush_scroll = 0.0f;
    float brush_screen_scale = 0.0f;
    bool reset_pending = false;
    bool panning = false;
};

SDL_AppResult Application::Initialize()
{
    if (!window.SetupWindow())
        return SDL_APP_FAILURE;

    view.SetZoom(GridView::DEFAULT_ZOOM, 0.5f, 0.5f);
    brush_screen_scale = controls.brush_size * view.zoom;
#ifdef __EMSCRIPTEN__
    if (!automata.Initialize(config::GRID_WIDTH, config::GRID_HEIGHT))
#else
    automata_storage = std::make_unique<AutomataGpu>(window.GetGPUDevice());
    if (!automata_storage->Initialize(renderer.GetSwapchainFormat(), config::GRID_WIDTH, config::GRID_HEIGHT))
#endif
        return ReportError("Initializing GPU simulation");

    imgui.CreateAndSetupContext();
    imgui.SetupScaling();
    if (!imgui.SetupBackends())
        return ReportError("Initializing ImGui");

    SDL_ShowWindow(window.GetSDLWindow());
    window.StartRun();
    previous_frame = SDL_GetTicksNS();
    return SDL_APP_CONTINUE;
}

SDL_AppResult Application::Frame()
{
    const Uint64 now = SDL_GetTicksNS();
    const double elapsed_seconds = static_cast<double>(now - previous_frame) / SDL_NS_PER_SECOND;
    previous_frame = now;
    events.ProcessEvents();
    if (!window.IsRunning())
        return SDL_APP_SUCCESS;
    if (SDL_GetWindowFlags(window.GetSDLWindow()) & SDL_WINDOW_MINIMIZED)
    {
        previous_cell.reset();
        panning = false;
        stroke_started = false;
        clock.Reset();
#ifndef __EMSCRIPTEN__
        SDL_Delay(10);
#endif
        return SDL_APP_CONTINUE;
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

#ifdef __EMSCRIPTEN__
    int pixel_width = 0;
    int pixel_height = 0;
    const bool can_draw = SDL_GetWindowSizeInPixels(window.GetSDLWindow(), &pixel_width, &pixel_height) &&
        pixel_width > 0 && pixel_height > 0;
    SDL_GPUCommandBuffer* commands = nullptr;
#else
    auto& automata = *automata_storage;
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
#endif
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
        if (!automata.Paint(commands, stroke, paint_hue, controls.brush_saturation,
                            controls.brush_size, stroke_seed))
        {
#ifndef __EMSCRIPTEN__
            // A command buffer with an acquired swapchain must be submitted.
            renderer.SubmitCommandBuffer();
#endif
            return ReportError("Uploading painted cells");
        }
#ifdef __EMSCRIPTEN__
        automata.Draw(pixel_width, pixel_height, view);
        imgui.Draw();
        if (!SDL_GL_SwapWindow(window.GetSDLWindow()))
            return ReportError("Presenting the WebGL frame");
#else
        imgui.Prepare(commands);
        renderer.SetColorTargetInfo(math::RGBAColor(0.0f, 0.0f, 0.0f, 1.0f));
        renderer.BeginRenderPass();
        auto* pass = renderer.GetRenderPass();
        automata.Draw(commands, pass, view);
        imgui.Draw(pass, commands);
        renderer.EndRenderPass();
#endif
    }

#ifndef __EMSCRIPTEN__
    if (!renderer.SubmitCommandBuffer())
        return ReportError("Submitting GPU frame");
#endif
    return SDL_APP_CONTINUE;
}
}

int main(int, char**)
{
    auto app = std::make_unique<Application>();
    auto result = app->Initialize();
    if (result != SDL_APP_CONTINUE)
        return 1;

#ifdef __EMSCRIPTEN__
    emscripten_set_main_loop_arg([](void* data) {
        auto* application = static_cast<Application*>(data);
        if (application->Frame() != SDL_APP_CONTINUE)
        {
            emscripten_cancel_main_loop();
            delete application;
        }
    }, app.release(), 0, false);
#else
    while ((result = app->Frame()) == SDL_APP_CONTINUE)
    {
    }
    if (result == SDL_APP_FAILURE)
        return 1;
#endif
    return 0;
}
