#include "shader_loader.hpp"

#include <SDL3/SDL.h>
#include <memory>
#include <string>

SDL_GPUShader* LoadSpirvShader(SDL_GPUDevice* device, const char* filename,
                              SDL_GPUShaderStage stage, Uint32 samplers, Uint32 uniforms)
{
    const char* base = SDL_GetBasePath();
    if (!base)
        return nullptr;

    const std::string path = std::string(base) + "shaders/" + filename;
    size_t size = 0;
    std::unique_ptr<void, decltype(&SDL_free)> bytes(SDL_LoadFile(path.c_str(), &size), SDL_free);
    if (!bytes)
        return nullptr;

    SDL_GPUShaderCreateInfo info{};
    info.code = static_cast<const Uint8*>(bytes.get());
    info.code_size = size;
    info.entrypoint = "main";
    info.format = SDL_GPU_SHADERFORMAT_SPIRV;
    info.stage = stage;
    info.num_samplers = samplers;
    info.num_uniform_buffers = uniforms;
    return SDL_CreateGPUShader(device, &info);
}
