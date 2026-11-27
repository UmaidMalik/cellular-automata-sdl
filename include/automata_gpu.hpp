#pragma once

#include <SDL3/SDL_gpu.h>
#include <cstdint>
#include <span>

struct SimulationControls;

class AutomataGpu
{
public:
    explicit AutomataGpu(SDL_GPUDevice* device);
    ~AutomataGpu();

    AutomataGpu(const AutomataGpu&) = delete;
    AutomataGpu& operator=(const AutomataGpu&) = delete;

    bool Initialize(SDL_GPUTextureFormat display_format, int width, int height,
                    int max_states, std::span<const uint8_t> seed);
    void Step(SDL_GPUCommandBuffer* commands, const SimulationControls& controls);
    void Draw(SDL_GPURenderPass* pass) const;

private:
    bool UploadSeed(std::span<const uint8_t> seed);

    SDL_GPUDevice* m_device;
    SDL_GPUTexture* m_current = nullptr;
    SDL_GPUTexture* m_next = nullptr;
    SDL_GPUSampler* m_sampler = nullptr;
    SDL_GPUGraphicsPipeline* m_display_pipeline = nullptr;
    SDL_GPUGraphicsPipeline* m_update_pipeline = nullptr;
    int m_width = 0;
    int m_height = 0;
    int m_max_states = 0;
};
