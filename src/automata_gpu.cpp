#include "automata_gpu.hpp"
#include "config.hpp"
#include "shader_loader.hpp"

#include <SDL3/SDL.h>
#include <cstring>
#include <utility>

namespace
{
struct alignas(16) AutomataParams
{
    int32_t width, height, range, wrap;
    int32_t max_states;
    float decay;
    int32_t padding[2];
    uint32_t birth_low, birth_high, survival_low, survival_high;
};
static_assert(sizeof(AutomataParams) == 48);
static_assert(sizeof(GpuCell) == 8);

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
    if (m_sampler)
        SDL_ReleaseGPUSampler(m_device, m_sampler);
    if (m_next)
        SDL_ReleaseGPUTexture(m_device, m_next);
    if (m_current)
        SDL_ReleaseGPUTexture(m_device, m_current);
}

bool AutomataGpu::Initialize(SDL_GPUTextureFormat display_format, int width, int height,
                             int max_states, std::span<const GpuCell> seed)
{
    m_width = width;
    m_height = height;
    m_max_states = max_states;

    SDL_GPUShader* vertex = LoadSpirvShader(
        m_device, "grid_fullscreen_triangle.vert.spv", SDL_GPU_SHADERSTAGE_VERTEX);
    if (!vertex)
        return false;

    constexpr auto state_format = SDL_GPU_TEXTUREFORMAT_R16G16B16A16_UNORM;
    m_display_pipeline = CreatePipeline(m_device, vertex, "grid_sample_rgba8.frag.spv", 0, display_format);
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
    return m_sampler && UploadSeed(seed);
}

bool AutomataGpu::UploadSeed(std::span<const GpuCell> seed)
{
    if (seed.size() != static_cast<size_t>(m_width) * m_height || seed.size() > UINT32_MAX / sizeof(GpuCell))
        return SDL_SetError("Seed size does not match the GPU grid");

    SDL_GPUTransferBufferCreateInfo info{};
    info.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD;
    info.size = static_cast<Uint32>(seed.size_bytes());
    auto* buffer = SDL_CreateGPUTransferBuffer(m_device, &info);
    if (!buffer)
        return false;

    bool submitted = false;
    if (void* data = SDL_MapGPUTransferBuffer(m_device, buffer, false))
    {
        std::memcpy(data, seed.data(), seed.size_bytes());
        SDL_UnmapGPUTransferBuffer(m_device, buffer);

        if (auto* commands = SDL_AcquireGPUCommandBuffer(m_device))
        {
            auto* pass = SDL_BeginGPUCopyPass(commands);
            SDL_GPUTextureTransferInfo source{};
            source.transfer_buffer = buffer;
            SDL_GPUTextureRegion destination{};
            destination.texture = m_current;
            destination.w = m_width;
            destination.h = m_height;
            destination.d = 1;
            SDL_UploadToGPUTexture(pass, &source, &destination, false);
            SDL_EndGPUCopyPass(pass);
            submitted = SDL_SubmitGPUCommandBuffer(commands);
        }
    }

    SDL_ReleaseGPUTransferBuffer(m_device, buffer);
    return submitted;
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
        m_max_states, controls.color_decay, {}, controls.birth_mask, 0, controls.survival_mask, 0
    };
    SDL_PushGPUFragmentUniformData(commands, 0, &params, sizeof(params));
    SDL_DrawGPUPrimitives(pass, 3, 1, 0, 0);
    SDL_EndGPURenderPass(pass);
    std::swap(m_current, m_next);
}

void AutomataGpu::Draw(SDL_GPURenderPass* pass) const
{
    SDL_BindGPUGraphicsPipeline(pass, m_display_pipeline);
    const SDL_GPUTextureSamplerBinding binding{m_current, m_sampler};
    SDL_BindGPUFragmentSamplers(pass, 0, &binding, 1);
    SDL_DrawGPUPrimitives(pass, 3, 1, 0, 0);
}
