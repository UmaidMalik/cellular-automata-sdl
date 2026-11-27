#pragma once

#include <GLES3/gl3.h>
#include <SDL3/SDL_gpu.h>
#include <SDL3/SDL_rect.h>
#include <array>
#include <span>

enum class BrushMode;
struct GridView;
struct SimulationControls;

class WebAutomata
{
public:
    ~WebAutomata();
    static int MaxGridSize();
    bool Initialize(int width, int height);
    bool Resize(int width, int height);
    void Clear(SDL_GPUCommandBuffer*);
    void Step(SDL_GPUCommandBuffer*, const SimulationControls& controls);
    bool Paint(SDL_GPUCommandBuffer*, std::span<const SDL_Point> stroke,
               float hue, int saturation, int size, Uint32 seed, BrushMode mode);
    void Draw(const SDL_Rect& viewport, int pixel_height, const GridView& view);

private:
    std::array<GLuint, 2> m_textures{};
    std::array<GLuint, 2> m_targets{};
    GLuint m_update = 0;
    GLuint m_display = 0;
    GLuint m_params = 0;
    GLuint m_view = 0;
    GLuint m_vao = 0;
    int m_width = 0;
    int m_height = 0;
    int m_current = 0;
};
