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

struct PinchSample
{
    SDL_TouchID device;
    SDL_FingerID first;
    SDL_FingerID second;
    SDL_FPoint center;
    float distance;
};

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
    SDL_Rect previous_viewport{};
    ImVec2 previous_screen{};
    bool block_touches_until_release = false;
#ifdef __EMSCRIPTEN__
    float previous_cell_pixels = 0.0f;
#endif
    SDL_Point grid{config::GRID_WIDTH, config::GRID_HEIGHT};
    bool grid_resize_failed = false;
    SimulationClock clock;
    Uint64 previous_frame = 0;
    std::optional<SDL_Point> previous_cell;
    std::optional<SDL_Point> pending_touch_cell;
    std::optional<PinchSample> pinch;
    Uint64 touch_start_time = 0;
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

    int width = 0;
    int height = 0;
    SDL_GetWindowSize(window.GetSDLWindow(), &width, &height);
    const auto viewport = FitGridViewport(width, height, GRID_ASPECT_RATIOS[view.aspect_index]);
    view.SetViewport(viewport, grid.x, grid.y);
    // On narrow screens, start close enough that a one-cell brush is visible.
    const float zoom = viewport.w > 0
        ? std::max(GridView::DEFAULT_ZOOM, std::ceil(grid.x * view.ExtentX() / viewport.w))
        : GridView::DEFAULT_ZOOM;
    view.SetZoom(zoom, 0.5f, 0.5f);
    brush_screen_scale = controls.brush_size * view.zoom;
#ifdef __EMSCRIPTEN__
    if (!automata.Initialize(grid.x, grid.y))
#else
    automata_storage = std::make_unique<AutomataGpu>(window.GetGPUDevice());
    if (!automata_storage->Initialize(renderer.GetSwapchainFormat(), grid.x, grid.y))
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
        pinch.reset();
        pending_touch_cell.reset();
        stroke_started = false;
        clock.Reset();
#ifndef __EMSCRIPTEN__
        SDL_Delay(10);
#endif
        return SDL_APP_CONTINUE;
    }

#ifdef __EMSCRIPTEN__
    window.SyncBrowserCanvas();
#endif
    imgui.BeginFrame();
    const float previous_zoom = view.zoom;
    const int previous_brush_size = controls.brush_size;
    const bool was_synced = controls.sync_brush_with_zoom;
    float zoom = view.zoom;
    const auto action = imgui.Build(controls, zoom, view.aspect_index, grid, grid_resize_failed);
    reset_pending = reset_pending || action == SimulationAction::Reset;
    const auto& io = *imgui.GetIO();
    const ImVec2 mouse = io.MousePos;
    const ImVec2 screen = io.DisplaySize;
    int pixel_width = 0;
    int pixel_height = 0;
    SDL_GetWindowSizeInPixels(window.GetSDLWindow(), &pixel_width, &pixel_height);
    if (action == SimulationAction::ResizeGrid)
    {
        const auto requested = GridSizeForAspect(controls.grid_size, GRID_ASPECT_RATIOS[view.aspect_index],
            {static_cast<int>(screen.x), static_cast<int>(screen.y)});
#ifdef __EMSCRIPTEN__
        grid_resize_failed = !automata.Resize(requested.x, requested.y);
#else
        grid_resize_failed = !automata_storage->Resize(requested.x, requested.y);
#endif
        if (grid_resize_failed)
            logger::Error("Resizing grid: {}", SDL_GetError());
        else
        {
            grid = requested;
            previous_cell.reset();
            clock.Reset();
            previous_frame = SDL_GetTicksNS();
        }
    }
    const SDL_Rect viewport = FitGridViewport(pixel_width, pixel_height,
                                              GRID_ASPECT_RATIOS[view.aspect_index]);
    const bool screen_resized = screen.x != previous_screen.x || screen.y != previous_screen.y;
    const bool viewport_resized = !SDL_RectsEqual(&viewport, &previous_viewport);
    const auto touches = events.Touches();
    if (screen_resized || viewport_resized)
        block_touches_until_release = !touches.empty();
    else if (touches.empty())
        block_touches_until_release = false;
    previous_viewport = viewport;
    previous_screen = screen;

    const bool aspect_changed = view.SetViewport(viewport, grid.x, grid.y);
    if (screen_resized || viewport_resized || aspect_changed)
    {
        previous_cell.reset();
        pending_touch_cell.reset();
        pinch.reset();
    }

    const bool has_viewport = viewport.w > 0 && viewport.h > 0 && screen.x > 0.0f && screen.y > 0.0f;
    const ImVec2 pixel_scale = has_viewport ? ImVec2(pixel_width / screen.x, pixel_height / screen.y) : ImVec2();
    float brush_zoom = previous_zoom;
#ifdef __EMSCRIPTEN__
    const float css_viewport_width = has_viewport ? viewport.w / pixel_scale.x : 0.0f;
    // Keep the same apparent cell size and centered content when the browser rotates or resizes.
    if (has_viewport && screen_resized && previous_cell_pixels > 0.0f &&
        action != SimulationAction::ResizeGrid && zoom == previous_zoom)
    {
        const float cell_pixels = css_viewport_width / (grid.x * view.ExtentX());
        view.SetZoom(zoom * previous_cell_pixels / cell_pixels, 0.5f, 0.5f);
        zoom = view.zoom;
        brush_screen_scale *= zoom / previous_zoom;
        brush_zoom = zoom;
    }
#endif
    if (zoom != view.zoom)
    {
        view.SetZoom(zoom, 0.5f, 0.5f);
        previous_cell.reset();
    }
    const auto viewport_position = [&](float x, float y) {
        return has_viewport ? SDL_FPoint{(x - viewport.x) / viewport.w, (y - viewport.y) / viewport.h}
                            : SDL_FPoint{-1.0f, -1.0f};
    };
    const auto inside_viewport = [](SDL_FPoint point) {
        return point.x >= 0.0f && point.y >= 0.0f && point.x < 1.0f && point.y < 1.0f;
    };
    const auto pointer = viewport_position(mouse.x * pixel_scale.x, mouse.y * pixel_scale.y);
    const bool erase = controls.erase_brush || (!io.WantTextInput && SDL_GetKeyboardState(nullptr)[SDL_SCANCODE_X]);
    stroke.clear();
    const bool pressed = ImGui::IsMouseClicked(ImGuiMouseButton_Left);
    const bool held = ImGui::IsMouseDown(ImGuiMouseButton_Left);
    const bool released = ImGui::IsMouseReleased(ImGuiMouseButton_Left);
    const bool touch_pointer = io.MouseSource == ImGuiMouseSource_TouchScreen;
    if (pressed || (!held && !released))
        stroke_started = false;

    const bool over_ui = io.WantCaptureMouse || ImGui::IsWindowHovered(ImGuiHoveredFlags_AnyWindow);
    const bool over_grid = !over_ui && inside_viewport(pointer);
    const bool block_touch_paint = block_touches_until_release ||
        (events.MultitouchSequence() && (touch_pointer || !touches.empty()));
    if (block_touch_paint)
    {
        pending_touch_cell.reset();
        previous_cell.reset();
        stroke_started = false;
    }

    if (has_viewport && !block_touches_until_release && touches.size() == 2 && touches[0].device == touches[1].device)
    {
        const auto& first = touches[0];
        const auto& second = touches[1];
        const bool same_pair = pinch && pinch->device == first.device &&
            pinch->first == first.id && pinch->second == second.id;
        const auto from_touch = [&](SDL_FPoint point) {
            return viewport_position(point.x * pixel_width, point.y * pixel_height);
        };
        if (same_pair || (!over_ui && inside_viewport(from_touch(first.origin)) &&
                          inside_viewport(from_touch(second.origin))))
        {
            const SDL_FPoint center{(first.position.x + second.position.x) * 0.5f,
                                    (first.position.y + second.position.y) * 0.5f};
            const float distance = std::hypot((first.position.x - second.position.x) * pixel_width,
                                              (first.position.y - second.position.y) * pixel_height);
            const PinchSample current{first.device, first.id, second.id, from_touch(center), distance};
            if (same_pair)
            {
                if (pinch->distance >= 1.0f && current.distance >= 1.0f)
                    view.SetZoom(view.zoom * (current.distance / pinch->distance), pinch->center.x, pinch->center.y);
                view.Pan(current.center.x - pinch->center.x, current.center.y - pinch->center.y);
            }
            pinch = current;
        }
        else
            pinch.reset();
    }
    else
        pinch.reset();

    if (!ImGui::IsMouseDown(ImGuiMouseButton_Right))
        panning = false;
    if (ImGui::IsMouseClicked(ImGuiMouseButton_Right))
        panning = over_grid;
    else if (panning && has_viewport)
        view.Pan(io.MouseDelta.x * pixel_scale.x / viewport.w, io.MouseDelta.y * pixel_scale.y / viewport.h);

    const bool can_edit_grid = over_grid && !panning && !block_touch_paint;
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
                         pointer.x, pointer.y);
            previous_cell.reset();
        }
    }

    if (!controls.sync_brush_with_zoom || !was_synced || controls.brush_size != previous_brush_size)
    {
        brush_screen_scale = controls.brush_size * view.zoom;
    }
    else if (view.zoom != brush_zoom)
    {
        // Keep the reference even when rounding or clamping the grid diameter.
        const int size = static_cast<int>(std::lround(brush_screen_scale / view.zoom));
        controls.brush_size = std::clamp(size, 1, SimulationControls::MAX_BRUSH_SIZE);
    }

    if (pressed)
    {
        previous_cell.reset();
        pending_touch_cell.reset();
        if (touch_pointer && can_edit_grid)
        {
            pending_touch_cell = view.CellAt(pointer.x, pointer.y, grid.x, grid.y);
            touch_start_time = now;
        }
    }
    // Wait briefly for a second finger before putting down the first cell.
    // A quick single-finger tap still paints when the finger lifts.
    const bool touch_tap = touch_pointer && released && pending_touch_cell.has_value();
    const bool touch_drag = touch_pointer && held && !touches.empty() &&
        now - touch_start_time >= SDL_MS_TO_NS(90);
    const bool paint = can_edit_grid && (touch_pointer ? touch_tap || touch_drag : held);
    if (paint)
    {
        if (!erase && !stroke_started)
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
        const SDL_Point cell = view.CellAt(pointer.x, pointer.y, grid.x, grid.y);
        AddStroke(stroke, previous_cell.value_or(pending_touch_cell.value_or(cell)), cell);
        previous_cell = cell;
        pending_touch_cell.reset();
    }
    if (released)
        pending_touch_cell.reset();
    if (stroke.empty())
        previous_cell.reset();
#ifdef __EMSCRIPTEN__
    if (has_viewport)
        previous_cell_pixels = css_viewport_width / (grid.x * view.ExtentX());
#endif
    imgui.EndFrame();

#ifdef __EMSCRIPTEN__
    const bool can_draw = has_viewport;
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

    const bool can_draw = has_viewport && renderer.GetSwapchainTexture() != nullptr;
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
            else if (!paused && action != SimulationAction::ResizeGrid)
                steps = clock.Advance(elapsed_seconds, controls.generations_per_second);

            for (int step = 0; step < steps; ++step)
                automata.Step(commands, controls);
        }
        if (!automata.Paint(commands, stroke, paint_hue, controls.brush_saturation,
                            controls.brush_size, stroke_seed, erase))
        {
#ifndef __EMSCRIPTEN__
            // A command buffer with an acquired swapchain must be submitted.
            renderer.SubmitCommandBuffer();
#endif
            return ReportError("Uploading painted cells");
        }
#ifdef __EMSCRIPTEN__
        automata.Draw(viewport, pixel_height, view);
        imgui.Draw();
        if (!SDL_GL_SwapWindow(window.GetSDLWindow()))
            return ReportError("Presenting the WebGL frame");
#else
        imgui.Prepare(commands);
        renderer.SetColorTargetInfo(math::RGBAColor(0.06f, 0.06f, 0.06f, 1.0f));
        renderer.BeginRenderPass();
        auto* pass = renderer.GetRenderPass();
        const SDL_GPUViewport grid_viewport{
            static_cast<float>(viewport.x), static_cast<float>(viewport.y),
            static_cast<float>(viewport.w), static_cast<float>(viewport.h), 0.0f, 1.0f
        };
        SDL_SetGPUViewport(pass, &grid_viewport);
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
