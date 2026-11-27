#include "automata_gpu.hpp"
#include "config.hpp"
#include "grid_view.hpp"
#include "shader_loader.hpp"

#include <SDL3/SDL.h>
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <utility>
#include <vector>

namespace
{
struct alignas(16) AutomataParams
{
    int32_t width, height, range, wrap;
    int32_t aging_stages;
    float decay;
    int32_t padding[2];
    uint32_t birth_low, birth_high, survival_low, survival_high;
};
static_assert(sizeof(AutomataParams) == 48);
using BrushTexel = std::array<uint16_t, 4>;
// Align both row pitch and source offset for SDL GPU backends such as D3D12.
constexpr Uint32 BRUSH_ROW_ALIGNMENT = 512;

bool BrushCellIsAlive(Uint32 seed, int x, int y)
{
    Uint32 hash = seed ^ (static_cast<Uint32>(x) * 0x9e3779b9u) ^ (static_cast<Uint32>(y) * 0x85ebca6bu);
    hash ^= hash >> 16;
    hash *= 0x7feb352du;
    hash ^= hash >> 15;
    hash *= 0x846ca68bu;
    return ((hash ^ (hash >> 16)) >> 31) != 0;
}
static_assert(sizeof(BrushTexel) == 8);

SDL_GPUGraphicsPipeline* CreatePipeline(SDL_GPUDevice* device, SDL_GPUShader* vertex,
    const char* fragment_name, Uint32 uniforms, SDL_GPUTextureFormat format)
{
    SDL_GPUShader* fragment = LoadSpirvShader(
        device, fragment_name, SDL_GPU_SHADERSTAGE_FRAGMENT, 1, uniforms);
    if (!fragment)
        return nullptr;

    SDL_GPUColorTargetDescription target{};
    target.format = format;

    SDL_GPUGraphicsPipelineCreateInfo info{};
    info.vertex_shader = vertex;
    info.fragment_shader = fragment;
    info.primitive_type = SDL_GPU_PRIMITIVETYPE_TRIANGLELIST;
    info.rasterizer_state.enable_depth_clip = true;
    info.target_info.color_target_descriptions = &target;
    info.target_info.num_color_targets = 1;

    SDL_GPUGraphicsPipeline* pipeline = SDL_CreateGPUGraphicsPipeline(device, &info);
    SDL_ReleaseGPUShader(device, fragment);
    return pipeline;
}
}

AutomataGpu::AutomataGpu(SDL_GPUDevice* device) : m_device(device)
{
}

AutomataGpu::~AutomataGpu()
{
    if (m_update_pipeline)
        SDL_ReleaseGPUGraphicsPipeline(m_device, m_update_pipeline);
    if (m_display_pipeline)
        SDL_ReleaseGPUGraphicsPipeline(m_device, m_display_pipeline);
    if (m_brush)
        SDL_ReleaseGPUTransferBuffer(m_device, m_brush);
    if (m_sampler)
        SDL_ReleaseGPUSampler(m_device, m_sampler);
    if (m_next)
        SDL_ReleaseGPUTexture(m_device, m_next);
    if (m_current)
        SDL_ReleaseGPUTexture(m_device, m_current);
}

bool AutomataGpu::Initialize(SDL_GPUTextureFormat display_format, int width, int height)
{
    m_width = width;
    m_height = height;

    SDL_GPUShader* vertex = LoadSpirvShader(
        m_device, "grid_fullscreen_triangle.vert.spv", SDL_GPU_SHADERSTAGE_VERTEX);
    if (!vertex)
        return false;

    constexpr auto state_format = SDL_GPU_TEXTUREFORMAT_R16G16B16A16_UNORM;
    m_display_pipeline = CreatePipeline(m_device, vertex, "grid_sample_rgba8.frag.spv", 1, display_format);
    if (m_display_pipeline)
        m_update_pipeline = CreatePipeline(m_device, vertex, "update_r1.frag.spv", 1, state_format);
    SDL_ReleaseGPUShader(m_device, vertex);
    if (!m_update_pipeline)
        return false;

    SDL_GPUTextureCreateInfo texture{};
    texture.type = SDL_GPU_TEXTURETYPE_2D;
    texture.format = state_format;
    texture.usage = SDL_GPU_TEXTUREUSAGE_COLOR_TARGET | SDL_GPU_TEXTUREUSAGE_SAMPLER;
    texture.width = width;
    texture.height = height;
    texture.layer_count_or_depth = 1;
    texture.num_levels = 1;

    m_current = SDL_CreateGPUTexture(m_device, &texture);
    if (!m_current)
        return false;
    m_next = SDL_CreateGPUTexture(m_device, &texture);
    if (!m_next)
        return false;

    SDL_GPUSamplerCreateInfo sampler{};
    sampler.min_filter = SDL_GPU_FILTER_NEAREST;
    sampler.mag_filter = SDL_GPU_FILTER_NEAREST;
    sampler.mipmap_mode = SDL_GPU_SAMPLERMIPMAPMODE_NEAREST;
    sampler.address_mode_u = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE;
    sampler.address_mode_v = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE;
    sampler.address_mode_w = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE;
    m_sampler = SDL_CreateGPUSampler(m_device, &sampler);
    if (!m_sampler)
        return false;

    auto* commands = SDL_AcquireGPUCommandBuffer(m_device);
    if (!commands)
        return false;
    Clear(commands);
    return SDL_SubmitGPUCommandBuffer(commands);
}

void AutomataGpu::Clear(SDL_GPUCommandBuffer* commands)
{
    // The update pass fully overwrites the second texture before it is read.
    SDL_GPUColorTargetInfo target{};
    target.texture = m_current;
    target.load_op = SDL_GPU_LOADOP_CLEAR;
    target.store_op = SDL_GPU_STOREOP_STORE;
    auto* pass = SDL_BeginGPURenderPass(commands, &target, 1, nullptr);
    SDL_EndGPURenderPass(pass);
}

bool AutomataGpu::Paint(SDL_GPUCommandBuffer* commands, std::span<const SDL_Point> stroke,
                        float hue, int size, Uint32 seed)
{
    if (stroke.empty())
        return true;

    const int half = size / 2;
    // Measure from cell centers in doubled coordinates so even diameters
    // can be centered between cells. Their outermost rows still get two cells.
    const int diameter = size - 1;
    const int radius_squared = diameter * diameter + (size % 2 == 0 ? 1 : 0);
    std::array<int, SimulationControls::MAX_BRUSH_SIZE> insets{};
    for (int y = 0; y < size; ++y)
    {
        const int dy = 2 * y - diameter;
        const int max_x = static_cast<int>(std::sqrt(radius_squared - dy * dy));
        insets[y] = (diameter - max_x + 1) / 2;
    }

    const auto bounds = std::ranges::minmax_element(stroke, {}, &SDL_Point::y);
    const int top = std::max(0, bounds.min->y - half);
    const int bottom = std::min(m_height, bounds.max->y - half + size);

    // The stroke centers form a contiguous line. Merge overlapping stamps
    // into one upload per row instead of issuing a copy for every cell.
    struct Row
    {
        int left;
        int right;
        Uint32 offset = 0;
        Uint32 pitch = 0;

        int Width() const { return right - left; }
    };
    std::vector<Row> rows(bottom - top, Row{m_width, 0});
    for (const auto cell : stroke)
    {
        const int stamp_left = cell.x - half;
        const int stamp_top = cell.y - half;
        const int first_row = std::max(0, stamp_top);
        const int end_row = std::min(m_height, stamp_top + size);

        for (int y = first_row; y < end_row; ++y)
        {
            const int inset = insets[y - stamp_top];
            const int left = std::max(0, stamp_left + inset);
            const int right = std::min(m_width, stamp_left + size - inset);
            if (left < right)
            {
                Row& row = rows[y - top];
                row.left = std::min(row.left, left);
                row.right = std::max(row.right, right);
            }
        }
    }

    Uint32 bytes = 0;
    for (auto& row : rows)
    {
        if (row.Width() <= 0)
            continue;
        const Uint32 row_bytes = row.Width() * sizeof(BrushTexel);
        row.offset = bytes;
        row.pitch = (row_bytes + BRUSH_ROW_ALIGNMENT - 1) / BRUSH_ROW_ALIGNMENT * BRUSH_ROW_ALIGNMENT;
        bytes += row.pitch;
    }

    if (bytes > m_brush_capacity)
    {
        const Uint32 capacity = std::max(bytes, m_brush_capacity * 2);
        const SDL_GPUTransferBufferCreateInfo info{SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD, capacity};
        auto* buffer = SDL_CreateGPUTransferBuffer(m_device, &info);
        if (!buffer)
            return false;

        if (m_brush)
            SDL_ReleaseGPUTransferBuffer(m_device, m_brush);
        m_brush = buffer;
        m_brush_capacity = capacity;
    }

    auto* data = static_cast<BrushTexel*>(SDL_MapGPUTransferBuffer(m_device, m_brush, true));
    if (!data)
        return false;

    const BrushTexel alive{
        static_cast<uint16_t>(std::lround(hue * UINT16_MAX)), UINT16_MAX, UINT16_MAX, UINT16_MAX
    };
    constexpr BrushTexel dead{};
    for (int y = top; y < bottom; ++y)
    {
        const auto& row = rows[y - top];
        auto* pixels = data + row.offset / sizeof(BrushTexel);
        for (int x = row.left; x < row.right; ++x)
            pixels[x - row.left] = BrushCellIsAlive(seed, x, y) ? alive : dead;
    }
    SDL_UnmapGPUTransferBuffer(m_device, m_brush);

    auto* pass = SDL_BeginGPUCopyPass(commands);
    SDL_GPUTextureTransferInfo source{};
    source.transfer_buffer = m_brush;
    source.rows_per_layer = 1;

    SDL_GPUTextureRegion destination{};
    destination.texture = m_current;
    destination.h = destination.d = 1;
    for (int y = top; y < bottom; ++y)
    {
        const auto& row = rows[y - top];
        if (row.Width() <= 0)
            continue;
        source.offset = row.offset;
        source.pixels_per_row = row.pitch / sizeof(BrushTexel);
        destination.x = row.left;
        destination.y = y;
        destination.w = row.Width();
        SDL_UploadToGPUTexture(pass, &source, &destination, false);
    }
    SDL_EndGPUCopyPass(pass);
    return true;
}

void AutomataGpu::Step(SDL_GPUCommandBuffer* commands, const SimulationControls& controls)
{
    SDL_GPUColorTargetInfo target{};
    target.texture = m_next;
    target.load_op = SDL_GPU_LOADOP_DONT_CARE;
    target.store_op = SDL_GPU_STOREOP_STORE;

    auto* pass = SDL_BeginGPURenderPass(commands, &target, 1, nullptr);
    SDL_BindGPUGraphicsPipeline(pass, m_update_pipeline);
    const SDL_GPUTextureSamplerBinding binding{m_current, m_sampler};
    SDL_BindGPUFragmentSamplers(pass, 0, &binding, 1);

    const AutomataParams params{
        m_width, m_height, SimulationControls::NEIGHBOR_RANGE, controls.wrap_edges,
        controls.aging_enabled ? controls.aging_stages : 0, controls.color_decay, {}, controls.birth_mask, 0, controls.survival_mask, 0
    };
    SDL_PushGPUFragmentUniformData(commands, 0, &params, sizeof(params));
    SDL_DrawGPUPrimitives(pass, 3, 1, 0, 0);
    SDL_EndGPURenderPass(pass);
    std::swap(m_current, m_next);
}

void AutomataGpu::Draw(SDL_GPUCommandBuffer* commands, SDL_GPURenderPass* pass, const GridView& view) const
{
    SDL_BindGPUGraphicsPipeline(pass, m_display_pipeline);
    const SDL_GPUTextureSamplerBinding binding{m_current, m_sampler};
    SDL_BindGPUFragmentSamplers(pass, 0, &binding, 1);
    const float viewport[]{view.left, view.top, 1.0f / view.zoom, 0.0f};
    SDL_PushGPUFragmentUniformData(commands, 0, viewport, sizeof(viewport));
    SDL_DrawGPUPrimitives(pass, 3, 1, 0, 0);
}
