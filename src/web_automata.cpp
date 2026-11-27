#include "web_automata.hpp"
#include "automata_parameters.hpp"
#include "brush.hpp"
#include "grid_view.hpp"
#include "web_shaders.hpp"

#include <SDL3/SDL.h>
#include <vector>

namespace
{
GLuint CompileShader(GLenum type, const char* source)
{
    const GLuint shader = glCreateShader(type);
    if (!shader)
        return 0;
    glShaderSource(shader, 1, &source, nullptr);
    glCompileShader(shader);
    GLint ok = GL_FALSE;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &ok);
    if (ok)
        return shader;

    char error[2048]{};
    glGetShaderInfoLog(shader, sizeof(error), nullptr, error);
    SDL_SetError("WebGL shader compilation failed: %s", error);
    glDeleteShader(shader);
    return 0;
}

GLuint BuildProgram(const char* fragment_source, const char* block_name)
{
    const GLuint vertex = CompileShader(GL_VERTEX_SHADER, web_shaders::vertex);
    if (!vertex)
        return 0;
    const GLuint fragment = CompileShader(GL_FRAGMENT_SHADER, fragment_source);
    if (!fragment)
    {
        glDeleteShader(vertex);
        return 0;
    }

    const GLuint program = glCreateProgram();
    glAttachShader(program, vertex);
    glAttachShader(program, fragment);
    glLinkProgram(program);
    glDeleteShader(vertex);
    glDeleteShader(fragment);

    GLint ok = GL_FALSE;
    glGetProgramiv(program, GL_LINK_STATUS, &ok);
    if (ok)
    {
        const GLuint block = glGetUniformBlockIndex(program, block_name);
        glUniformBlockBinding(program, block, 0);
        return program;
    }

    char error[2048]{};
    glGetProgramInfoLog(program, sizeof(error), nullptr, error);
    SDL_SetError("WebGL program link failed: %s", error);
    glDeleteProgram(program);
    return 0;
}

void UseTexture(GLuint program, GLuint buffer, GLuint vao, GLuint texture)
{
    glUseProgram(program);
    glBindBufferBase(GL_UNIFORM_BUFFER, 0, buffer);
    glBindVertexArray(vao);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, texture);
}
}

WebAutomata::~WebAutomata()
{
    if (m_update)
        glDeleteProgram(m_update);
    if (m_display)
        glDeleteProgram(m_display);
    if (m_params)
        glDeleteBuffers(1, &m_params);
    if (m_view)
        glDeleteBuffers(1, &m_view);
    if (m_vao)
        glDeleteVertexArrays(1, &m_vao);
    if (m_targets[0])
        glDeleteFramebuffers(m_targets.size(), m_targets.data());
    if (m_textures[0])
        glDeleteTextures(m_textures.size(), m_textures.data());
}

bool WebAutomata::Initialize(int width, int height)
{
    m_width = width;
    m_height = height;
    m_update = BuildProgram(web_shaders::update, "Params");
    if (!m_update)
        return false;
    m_display = BuildProgram(web_shaders::display, "View");
    if (!m_display)
        return false;

    glGenVertexArrays(1, &m_vao);
    glGenBuffers(1, &m_params);
    glGenBuffers(1, &m_view);
    glGenTextures(m_textures.size(), m_textures.data());
    glGenFramebuffers(m_targets.size(), m_targets.data());

    for (size_t i = 0; i < m_textures.size(); ++i)
    {
        glBindTexture(GL_TEXTURE_2D, m_textures[i]);
        glTexStorage2D(GL_TEXTURE_2D, 1, GL_RGBA16UI, width, height);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glBindFramebuffer(GL_FRAMEBUFFER, m_targets[i]);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_textures[i], 0);
        if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
            return SDL_SetError("Could not create a %dx%d WebGL cell texture", width, height);
    }
    Clear(nullptr);
    return true;
}

void WebAutomata::Clear(SDL_GPUCommandBuffer*)
{
    glBindFramebuffer(GL_FRAMEBUFFER, m_targets[m_current]);
    constexpr GLuint dead[4]{};
    glClearBufferuiv(GL_COLOR, 0, dead);
}

void WebAutomata::Step(SDL_GPUCommandBuffer*, const SimulationControls& controls)
{
    const auto params = MakeAutomataParams(m_width, m_height, controls);
    glBindBuffer(GL_UNIFORM_BUFFER, m_params);
    glBufferData(GL_UNIFORM_BUFFER, sizeof(params), &params, GL_DYNAMIC_DRAW);
    const int next = 1 - m_current;
    glBindFramebuffer(GL_FRAMEBUFFER, m_targets[next]);
    glViewport(0, 0, m_width, m_height);
    UseTexture(m_update, m_params, m_vao, m_textures[m_current]);
    glDrawArrays(GL_TRIANGLES, 0, 3);
    m_current = next;
}

bool WebAutomata::Paint(SDL_GPUCommandBuffer*, std::span<const SDL_Point> stroke,
                       float hue, int saturation, int size, Uint32 seed)
{
    if (stroke.empty())
        return true;

    glBindTexture(GL_TEXTURE_2D, m_textures[m_current]);
    const auto alive = MakeBrushTexel(hue, saturation);
    std::vector<BrushTexel> pixels;
    for (const auto& row : RasterizeBrush(stroke, size, m_width, m_height))
    {
        if (row.Width() <= 0)
            continue;
        pixels.resize(row.Width());
        FillBrushRow(pixels.data(), row, seed, alive);
        glTexSubImage2D(GL_TEXTURE_2D, 0, row.left, row.y, row.Width(), 1,
                        GL_RGBA_INTEGER, GL_UNSIGNED_SHORT, pixels.data());
    }
    const GLenum error = glGetError();
    return error == GL_NO_ERROR || SDL_SetError("WebGL brush upload failed (0x%x)", error);
}

void WebAutomata::Draw(int pixel_width, int pixel_height, const GridView& view)
{
    const float viewport[]{view.left, view.top, 1.0f / view.zoom, 0.0f};
    glBindBuffer(GL_UNIFORM_BUFFER, m_view);
    glBufferData(GL_UNIFORM_BUFFER, sizeof(viewport), viewport, GL_DYNAMIC_DRAW);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glViewport(0, 0, pixel_width, pixel_height);
    UseTexture(m_display, m_view, m_vao, m_textures[m_current]);
    glDrawArrays(GL_TRIANGLES, 0, 3);
}
