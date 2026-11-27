#include "brush.hpp"
#include "config.hpp"
#include "grid_view.hpp"
#include "rule_presets.hpp"
#include "imgui_manager.hpp"
#include "imgui.h"
#include "imgui_impl_sdl3.h"
#ifdef __EMSCRIPTEN__
#include <emscripten/emscripten.h>
#include "imgui_impl_opengl3.h"
#else
#include "imgui_impl_sdlgpu3.h"
#endif
#include "window.hpp"
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <iostream>
#include <string>

ImGuiManager::ImGuiManager(Window* window) 
    : m_window(window)
{
    if (!m_window)
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

#ifndef __EMSCRIPTEN__
    if (m_window && m_window->GetSDLWindow())
    {
        if (m_window->GetGPUDevice())
            SDL_WaitForGPUIdle(m_window->GetGPUDevice());
    }
#endif

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

#ifdef __EMSCRIPTEN__
    if (!ImGui_ImplSDL3_InitForOpenGL(m_window->GetSDLWindow(), m_window->GetGLContext()))
        return false;
    if (!ImGui_ImplOpenGL3_Init("#version 300 es"))
    {
        ImGui_ImplSDL3_Shutdown();
        return false;
    }
#else
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
#endif
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
#ifdef __EMSCRIPTEN__
    ImGui_ImplOpenGL3_Shutdown();
#else
    ImGui_ImplSDLGPU3_Shutdown();
#endif
    ImGui_ImplSDL3_Shutdown();
}

void ImGuiManager::BeginFrame()
{
#ifdef __EMSCRIPTEN__
    ImGui_ImplOpenGL3_NewFrame();
#else
    ImGui_ImplSDLGPU3_NewFrame();
#endif
    ImGui_ImplSDL3_NewFrame();
    ImGui::NewFrame();
}

namespace
{
void EditRule(const char* label, NeighborRule& mask, int max_neighbors, bool mobile_layout)
{
    ImGui::TextUnformatted(label);
    ImGui::PushID(label);

    const bool scroll = !mobile_layout && max_neighbors > 8;
    const bool table = scroll || mobile_layout;
    if (scroll)
    {
        const float row_height = ImGui::GetFrameHeightWithSpacing();
        ImGui::BeginChild("Counts scroll", ImVec2(row_height * 20.0f, row_height * 3.6f),
                          ImGuiChildFlags_Borders);
    }

    if (!table || ImGui::BeginTable("Counts", mobile_layout ? 6 : 8, ImGuiTableFlags_SizingStretchSame))
    {
        for (int count = 0; count <= max_neighbors; ++count)
        {
            if (table)
                ImGui::TableNextColumn();
            else if (count > 0)
                ImGui::SameLine();

            char text[4];
            std::snprintf(text, sizeof(text), "%d", count);
            ImGui::CheckboxFlags(text, &mask[count / 32], 1u << (count % 32));
        }
        if (table)
            ImGui::EndTable();
    }

    if (scroll)
        ImGui::EndChild();
    ImGui::PopID();
}

std::string DescribeRuleCounts(const NeighborRule& mask, int max_count)
{
    std::string text;
    for (int first = 0; first <= max_count; ++first)
    {
        if (!RuleHasCount(mask, first))
            continue;

        int last = first;
        while (last < max_count && RuleHasCount(mask, last + 1))
            ++last;
        if (!text.empty())
            text += ", ";
        text += std::to_string(first);
        if (last > first)
        {
            text += "-";
            text += std::to_string(last);
        }
        first = last;
    }
    return text.empty() ? "none" : text;
}

void EditSimulationRule(SimulationControls& controls, bool mobile_layout)
{
    const auto* active = FindActiveRulePreset(controls);
    if (ImGui::BeginCombo("Rule preset", active ? active->name : "Custom"))
    {
        const char* const groups[]{"Life-like", "Generations", "Larger than Life"};
        int previous_group = -1;
        for (const auto& preset : RULE_PRESETS)
        {
            const int group = preset.radius > 1 ? 2 : (preset.aging_stages > 0 ? 1 : 0);
            if (group != previous_group)
            {
                ImGui::SeparatorText(groups[group]);
                previous_group = group;
            }
            const bool selected = &preset == active;
            if (ImGui::Selectable(preset.name, selected))
                ApplyRulePreset(controls, preset);
            if (selected)
                ImGui::SetItemDefaultFocus();
        }
        ImGui::EndCombo();
    }

    const int width = NeighborhoodWidth(controls.neighbor_radius);
    const int max_count = NeighborhoodMaxCount(controls.neighbor_radius);
    ImGui::TextWrapped("Birth (B): %s   Survival (S): %s",
        DescribeRuleCounts(controls.birth_mask, max_count).c_str(),
        DescribeRuleCounts(controls.survival_mask, max_count).c_str());
    if (controls.aging_enabled)
        ImGui::Text("%d x %d Moore | Aging on, max %d", width, width, controls.aging_stages);
    else
        ImGui::Text("%d x %d Moore | Aging off", width, width);

    if (mobile_layout && !ImGui::CollapsingHeader("Customize rule"))
        return;

    const char* const neighborhoods[]{"3 x 3", "5 x 5", "7 x 7", "9 x 9", "11 x 11"};
    static_assert(IM_ARRAYSIZE(neighborhoods) == SimulationControls::MAX_NEIGHBOR_RADIUS);
    int neighborhood = controls.neighbor_radius - 1;
    if (ImGui::Combo("Neighborhood", &neighborhood, neighborhoods, IM_ARRAYSIZE(neighborhoods)))
        controls.neighbor_radius = neighborhood + 1;
    const int editable_count = NeighborhoodMaxCount(controls.neighbor_radius);
    EditRule("Birth neighbors", controls.birth_mask, editable_count, mobile_layout);
    EditRule("Survival neighbors", controls.survival_mask, editable_count, mobile_layout);
    ImGui::Checkbox("Aging states", &controls.aging_enabled);
    ImGui::BeginDisabled(!controls.aging_enabled);
    ImGui::SliderInt("Max aging stages", &controls.aging_stages, 1, SimulationControls::MAX_AGING_STAGES,
                     "%d", ImGuiSliderFlags_AlwaysClamp);
    ImGui::EndDisabled();
}

void EditBrushHue(SimulationControls& controls)
{
    ImGui::BeginDisabled(controls.random_brush_hue);
    ImGui::Text("Brush hue %.2f", controls.brush_hue);
    ImGui::SameLine();
    const float saturation = controls.brush_saturation / 100.0f;
    const auto hue_color = [saturation](float hue) {
        ImVec4 color(0.0f, 0.0f, 0.0f, 1.0f);
        ImGui::ColorConvertHSVtoRGB(hue, saturation, 1.0f, color.x, color.y, color.z);
        return color;
    };
    ImGui::ColorButton("##brush-color", hue_color(controls.brush_hue),
                       ImGuiColorEditFlags_NoTooltip | ImGuiColorEditFlags_NoDragDrop,
                       ImVec2(ImGui::GetTextLineHeight(), ImGui::GetTextLineHeight()));

    ImGui::SetNextItemWidth(-1.0f);
    const ImVec2 start = ImGui::GetCursorScreenPos();
    const float width = ImGui::CalcItemWidth();
    const float height = ImGui::GetFrameHeight();
    constexpr int segments = 6;
    for (int i = 0; i < segments; ++i)
    {
        const float left = static_cast<float>(i) / segments;
        const float right = static_cast<float>(i + 1) / segments;
        const ImU32 left_color = ImGui::GetColorU32(hue_color(left));
        const ImU32 right_color = ImGui::GetColorU32(hue_color(right));
        ImGui::GetWindowDrawList()->AddRectFilledMultiColor(
            ImVec2(start.x + width * left, start.y),
            ImVec2(start.x + width * right, start.y + height),
            left_color, right_color, right_color, left_color);
    }

    ImGui::PushStyleColor(ImGuiCol_FrameBg, IM_COL32(0, 0, 0, 0));
    ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, IM_COL32(0, 0, 0, 0));
    ImGui::PushStyleColor(ImGuiCol_FrameBgActive, IM_COL32(0, 0, 0, 0));
    ImGui::PushStyleColor(ImGuiCol_SliderGrab, IM_COL32(30, 30, 30, 255));
    ImGui::PushStyleColor(ImGuiCol_SliderGrabActive, IM_COL32(0, 0, 0, 255));
    ImGui::SliderFloat("##brush-hue", &controls.brush_hue, 0.0f, 1.0f, "",
                       ImGuiSliderFlags_NoInput | ImGuiSliderFlags_AlwaysClamp);
    ImGui::PopStyleColor(5);
    ImGui::EndDisabled();
}

void EditColorRetention(float& retention, bool mobile_layout)
{
    // V scales by retention each generation, so trail length is sensitive to
    // tiny changes near 1. Space the brightness loss (1 - retention) logarithmically.
    constexpr float loss_scale = 10000.0f;
    const float log_range = std::log1p(loss_scale);
    float position = 1.0f - std::log1p(loss_scale * (1.0f - retention)) / log_range;

    if (mobile_layout)
    {
        ImGui::TextUnformatted("Color retention");
        ImGui::SetNextItemWidth(-1.0f);
    }
    if (ImGui::SliderFloat(mobile_layout ? "##retention" : "Color retention", &position, 0.0f, 1.0f, "",
                           ImGuiSliderFlags_NoInput | ImGuiSliderFlags_NoRoundToFormat))
    {
        retention = std::clamp(
            1.0f - std::expm1((1.0f - position) * log_range) / std::expm1(log_range), 0.0f, 1.0f);
    }
    if (!mobile_layout)
        ImGui::SameLine();
    ImGui::Text("%.4f (%.2f%%)", retention, 100.0f * retention);
}
}

SimulationAction ImGuiManager::Build(SimulationControls& controls, float& zoom, int& aspect_index,
                                     SDL_Point grid, int max_grid_size, bool resize_failed)
{
    auto action = SimulationAction::None;
    const ImVec2 screen = m_imgui_io->DisplaySize;
    bool mobile_layout = false;
#ifdef __EMSCRIPTEN__
    static const bool touch_device = EM_ASM_INT({ return matchMedia("(pointer: coarse)").matches; });
    mobile_layout = touch_device || screen.y > screen.x;
    if (mobile_layout)
    {
        constexpr float margin = 8.0f;
        const int toolbar_action = EM_ASM_INT({
            const pending = Module['mobileToolbarAction'] || 0;
            Module['mobileToolbarAction'] = 0;
            return pending;
        });
        if (toolbar_action == 1)
            controls.erase_brush = false;
        else if (toolbar_action == 2)
            controls.erase_brush = true;
        else if (toolbar_action == 3)
            m_mobile_settings_open = !m_mobile_settings_open;

        // The browser shows only the fullscreen canvas. Keep equivalent
        // controls inside it when the HTML toolbar is outside that surface.
        if (EM_ASM_INT({ return document.fullscreenElement === Module.canvas; }))
        {
            ImGui::SetNextWindowPos(ImVec2(screen.x * 0.5f, screen.y - margin),
                                    ImGuiCond_Always, ImVec2(0.5f, 1.0f));
            if (ImGui::Begin("Fullscreen brush toolbar", nullptr,
                             ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoMove |
                             ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_AlwaysAutoResize))
            {
                const ImVec2 touch_target{76.0f, 40.0f};
                if (ImGui::Selectable("Draw", !controls.erase_brush, 0, touch_target))
                    controls.erase_brush = false;
                ImGui::SameLine();
                if (ImGui::Selectable("Erase", controls.erase_brush, 0, touch_target))
                    controls.erase_brush = true;
                ImGui::SameLine();
                if (ImGui::Button(m_mobile_settings_open ? "Close" : "Settings", touch_target))
                    m_mobile_settings_open = !m_mobile_settings_open;
            }
            ImGui::End();
        }

        EM_ASM({
            if (Module['syncMobileToolbar'])
                Module['syncMobileToolbar'](!!$0, !!$1);
        }, controls.erase_brush ? 1 : 0, m_mobile_settings_open ? 1 : 0);

        if (!m_mobile_settings_open)
            return action;

        ImGui::SetNextWindowPos(ImVec2(margin, margin), ImGuiCond_Always);
        ImGui::SetNextWindowSize(ImVec2(std::clamp(screen.x - 2.0f * margin, 1.0f, 460.0f),
                                       std::max(1.0f, screen.y - 82.0f)), ImGuiCond_Always);
    }
#endif

    const ImGuiWindowFlags flags = mobile_layout
        ? ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize |
          ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse
        : ImGuiWindowFlags_AlwaysAutoResize;
    const float fps = m_imgui_io->Framerate;
    char desktop_title[64];
    std::snprintf(desktop_title, sizeof(desktop_title), "Simulation  |  %.1f FPS###Simulation", fps);
    bool show_controls = ImGui::Begin(mobile_layout ? "Mobile settings" : desktop_title, nullptr, flags);
    const bool settings_child = mobile_layout && show_controls;
    if (settings_child)
    {
        if (ImGui::Button("Close settings", ImVec2(-1.0f, 40.0f)))
            m_mobile_settings_open = false;
        ImGui::Text("%.1f FPS", fps);
        ImGui::PushStyleVar(ImGuiStyleVar_ScrollbarSize, 24.0f);
        show_controls = ImGui::BeginChild("Settings controls");
    }

    if (show_controls)
    {
        ImGui::Checkbox("Paused", &controls.paused);
        ImGui::SameLine();
        ImGui::BeginDisabled(!controls.paused);
        if (ImGui::Button("Step"))
            action = SimulationAction::Step;
        ImGui::EndDisabled();
        ImGui::SameLine();
        if (ImGui::Button("Reset"))
            action = SimulationAction::Reset;

        ImGui::Checkbox("Pause on draw", &controls.pause_on_draw);
        ImGui::SetItemTooltip("Pause automatic generations while painting.");
        ImGui::SliderInt("Speed", &controls.generations_per_second,
                         SimulationControls::MIN_GENERATIONS_PER_SECOND,
                         SimulationControls::MAX_GENERATIONS_PER_SECOND,
                         "%d gen/s", ImGuiSliderFlags_AlwaysClamp);
        ImGui::Checkbox("Wrap edges", &controls.wrap_edges);
        bool fullscreen = (SDL_GetWindowFlags(m_window->GetSDLWindow()) & SDL_WINDOW_FULLSCREEN) != 0;
        if (ImGui::Checkbox("Fullscreen", &fullscreen))
            m_window->SetFullscreen(fullscreen);

        EditSimulationRule(controls, mobile_layout);
        if (ImGui::BeginCombo("Aspect ratio", GRID_ASPECT_RATIOS[aspect_index].label))
        {
            for (int index = 0; index < IM_ARRAYSIZE(GRID_ASPECT_RATIOS); ++index)
            {
                const bool selected = index == aspect_index;
                if (ImGui::Selectable(GRID_ASPECT_RATIOS[index].label, selected))
                    aspect_index = index;
                if (selected)
                    ImGui::SetItemDefaultFocus();
            }
            ImGui::EndCombo();
        }
        ImGui::SliderInt("Grid size", &controls.grid_size, SimulationControls::MIN_GRID_SIZE,
                         max_grid_size, "%d cells on long side", ImGuiSliderFlags_AlwaysClamp);
        const auto grid_target = GridSizeForAspect(controls.grid_size, GRID_ASPECT_RATIOS[aspect_index],
            {static_cast<int>(m_imgui_io->DisplaySize.x), static_cast<int>(m_imgui_io->DisplaySize.y)});
        ImGui::Text("Grid: %d x %d cells", grid.x, grid.y);
        ImGui::BeginDisabled(grid_target.x == grid.x && grid_target.y == grid.y);
        if (ImGui::Button("Apply grid size"))
            action = SimulationAction::ResizeGrid;
        ImGui::EndDisabled();
        if (grid_target.x != grid.x || grid_target.y != grid.y)
        {
            ImGui::SameLine();
            ImGui::TextWrapped("%d x %d cells", grid_target.x, grid_target.y);
        }
        if (resize_failed)
            ImGui::TextWrapped("Could not resize the grid. Try a smaller size.");

        ImGui::SliderFloat("Zoom", &zoom, GridView::MIN_ZOOM, GridView::MAX_ZOOM,
                           "%.1fx", ImGuiSliderFlags_Logarithmic | ImGuiSliderFlags_AlwaysClamp);
        ImGui::SliderInt("Brush size", &controls.brush_size, 1, MaxBrushSize(grid.x),
                         "%d cells", ImGuiSliderFlags_Logarithmic | ImGuiSliderFlags_AlwaysClamp);
        ImGui::SetItemTooltip("Shift + scroll over the grid to resize the brush.");
        ImGui::Checkbox("Solid brush", &controls.solid_brush);
        ImGui::SetItemTooltip("Fill the circle with live cells. When off, each cell is randomly set alive or dead.");
        ImGui::Checkbox("Erase brush", &controls.erase_brush);
        ImGui::SameLine();
        ImGui::TextWrapped("Hold X while drawing to erase.");
        ImGui::Checkbox("Scale brush with zoom", &controls.sync_brush_with_zoom);
        ImGui::SetItemTooltip("Keep the brush diameter roughly constant on screen while zooming.");
        ImGui::Checkbox("Random hue per stroke", &controls.random_brush_hue);
        EditBrushHue(controls);
        ImGui::SliderInt("Brush saturation", &controls.brush_saturation,
                         SimulationControls::MIN_BRUSH_SATURATION, SimulationControls::MAX_BRUSH_SATURATION,
                         "%d%%", ImGuiSliderFlags_AlwaysClamp);
        EditColorRetention(controls.color_decay, mobile_layout);
    }
    if (settings_child)
    {
        const auto& io = *m_imgui_io;
        if (io.MouseSource != ImGuiMouseSource_TouchScreen || !ImGui::IsMouseDown(ImGuiMouseButton_Left))
            m_mobile_scroll_touch = false;
        else if (ImGui::IsMouseClicked(ImGuiMouseButton_Left))
        {
            m_mobile_scroll_touch = ImGui::IsWindowHovered(ImGuiHoveredFlags_ChildWindows) &&
                                    !ImGui::IsAnyItemActive();
        }

        if (m_mobile_scroll_touch && ImGui::IsMouseDragging(ImGuiMouseButton_Left))
        {
            const auto drag = ImGui::GetMouseDragDelta();
            if (std::abs(drag.y) > std::abs(drag.x))
                ImGui::SetScrollY(ImGui::GetScrollY() - io.MouseDelta.y);
        }
        ImGui::EndChild();
        ImGui::PopStyleVar();
    }
    ImGui::End();
    return action;
}

#ifdef __EMSCRIPTEN__
void ImGuiManager::Draw()
{
    ImGui_ImplOpenGL3_RenderDrawData(m_draw_data);
}
#else
void ImGuiManager::Prepare(SDL_GPUCommandBuffer* command_buffer)
{
    ImGui_ImplSDLGPU3_PrepareDrawData(m_draw_data, command_buffer);

}

void ImGuiManager::Draw(SDL_GPURenderPass* render_pass, SDL_GPUCommandBuffer* command_buffer)
{
    ImGui_ImplSDLGPU3_RenderDrawData(m_draw_data, command_buffer, render_pass);
}

#endif

void ImGuiManager::EndFrame()
{
    ImGui::Render();
    m_draw_data = ImGui::GetDrawData();
}

ImDrawData* ImGuiManager::GetDrawData()
{
    return m_draw_data;
}
