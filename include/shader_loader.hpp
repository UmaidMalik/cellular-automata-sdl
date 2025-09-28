#pragma once

#include <unordered_map>
#include <SDL3/SDL.h>
#include <SDL3/SDL_gpu.h>

#include <string>
#include <array>
#include <vector>

class ShaderLoader {
    private:
        std::unordered_map<std::string, SDL_GPUShader*> m_shader_map;
        std::array<unsigned int, 6> m_shader_formats;
    public:
        ShaderLoader();
        void Load();
        void CreateShader();
        SDL_GPUShader* GetShader(std::string shader_name);
    private:
        std::vector<uint8_t> ReadFile(const char* path);

};

ShaderLoader::ShaderLoader()
    : m_shader_formats
    {
        SDL_GPU_SHADERFORMAT_INVALID
    }
{
    
}

std::vector<uint8_t> ShaderLoader::ReadFile(const char* path)
{
    std::FILE* file = std::fopen(path, "rb");
    if (!file)
    {
        std::cerr << "Warning: File not found: " << path << "\n";
        return {};
    }

    std::fseek(file, 0, SEEK_END);
    long size = std::ftell(file);
    std::fseek(file, 0, SEEK_SET);

    std::vector<uint8_t> buffer((size_t)size);
    if (size > 0)
    {
        std::fread(buffer.data(), 1, (size_t)size, file);
    }
    
    std::fclose(file);
    return buffer;
}