#pragma once

#include <SDL3/SDL_gpu.h>

SDL_GPUShader* LoadSpirvShader(SDL_GPUDevice* device, const char* filename,
                              SDL_GPUShaderStage stage, Uint32 samplers = 0,
                              Uint32 uniforms = 0);
