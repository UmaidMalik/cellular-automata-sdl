#pragma once

#include <SDL3/SDL_gpu.h>
#include <SDL3/SDL_rect.h>
#include <span>

struct SimulationControls;

struct GridView;

class AutomataGpu
{
public:
    explicit AutomataGpu(SDL_GPUDevice* device);
    ~AutomataGpu();

    AutomataGpu(const AutomataGpu&) = delete;
    AutomataGpu& operator=(const AutomataGpu&) = delete;

    bool Initialize(SDL_GPUTextureFormat display_format, int width, int height);
    bool Resize(int width, int height);
    void Clear(SDL_GPUCommandBuffer* commands);
    void Step(SDL_GPUCommandBuffer* commands, const SimulationControls& controls);
    bool Paint(SDL_GPUCommandBuffer* commands, std::span<const SDL_Point> stroke, float hue, int saturation, int size, Uint32 seed, bool erase);
    void Draw(SDL_GPUCommandBuffer* commands, SDL_GPURenderPass* pass, const GridView& view) const;

private:
    SDL_GPUDevice* m_device;
    SDL_GPUTexture* m_current = nullptr;
    SDL_GPUTexture* m_next = nullptr;
    SDL_GPUTransferBuffer* m_brush = nullptr;
    SDL_GPUSampler* m_sampler = nullptr;
    SDL_GPUGraphicsPipeline* m_display_pipeline = nullptr;
    SDL_GPUGraphicsPipeline* m_update_pipeline = nullptr;
    int m_width = 0;
    int m_height = 0;
    Uint32 m_brush_capacity = 0;
};
