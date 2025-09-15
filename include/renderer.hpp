#pragma once

#include "imgui_impl_sdl3.h"
#include "imgui_impl_sdlgpu3.h"
#include "math.hpp"
#include "window.hpp"
#include <iostream>

class Renderer
{
    private:
        SDL_GPUCommandBuffer* m_command_buffer;
        SDL_GPUTexture* m_swapchain_texture;
        SDL_GPUColorTargetInfo m_target_info{};
        SDL_GPURenderPass* m_render_pass;
        Window* m_window;
    public:
        Renderer(Window* window);
        ~Renderer();
        void AcquireCommandBuffer();
        SDL_GPUCommandBuffer* GetCommandBuffer();
        void AcquireSwapchainTexture();
        SDL_GPUTexture* GetSwapchainTexture();
        void SetColorTargetInfo(math::RGBAColor clear_color);
        void BeginRenderPass();
        SDL_GPURenderPass* GetRenderPass();
        void EndRenderPass();
        void SubmitCommandBuffer();

};

Renderer::Renderer(Window* window)
    : m_window(window)
{

}

Renderer::~Renderer()
{
    std::cout << "~Renderer() destructor called and exited\n";
}

void Renderer::AcquireCommandBuffer()
{
    m_command_buffer = SDL_AcquireGPUCommandBuffer(m_window->GetGPUDevice());
}

SDL_GPUCommandBuffer* Renderer::GetCommandBuffer()
{
    return m_command_buffer;
}

void Renderer::AcquireSwapchainTexture()
{
    SDL_WaitAndAcquireGPUSwapchainTexture(m_command_buffer, m_window->GetSDLWindow(), &m_swapchain_texture, nullptr, nullptr);
}

SDL_GPUTexture* Renderer::GetSwapchainTexture()
{
    return m_swapchain_texture;
}

void Renderer::SetColorTargetInfo(math::RGBAColor clear_color)
{
    m_target_info = {};
    m_target_info.texture = m_swapchain_texture;
    m_target_info.clear_color = SDL_FColor { clear_color.r, clear_color.g, clear_color.b, clear_color.a }; // ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);
    m_target_info.load_op = SDL_GPU_LOADOP_CLEAR;
    m_target_info.store_op = SDL_GPU_STOREOP_STORE;
    m_target_info.mip_level = 0;
    m_target_info.layer_or_depth_plane = 0;
    m_target_info.cycle = false;
}

void Renderer::BeginRenderPass()
{
    m_render_pass = SDL_BeginGPURenderPass(m_command_buffer, &m_target_info, 1, nullptr);
}

SDL_GPURenderPass* Renderer::GetRenderPass()
{
    return m_render_pass;
}

void Renderer::EndRenderPass()
{
    SDL_EndGPURenderPass(m_render_pass);
}

void Renderer::SubmitCommandBuffer()
{
    SDL_SubmitGPUCommandBuffer(m_command_buffer);
}