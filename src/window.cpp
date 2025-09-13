#include "window.hpp"
#include <SDL3/SDL.h>
#include <iostream>

Window::Window(int width, int height, const char* title)
    : m_width(width), m_height(height), m_window_title(title)
{
    std::cout << "Window created\n";
}

Window::~Window()
{
    if (m_sdl_window)
    {
        SDL_DestroyWindow(m_sdl_window);
        m_sdl_window = nullptr;
    }
    SDL_Quit();
}

int Window::InitWindow()
{
    if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_GAMEPAD))
    {
        SDL_Log("Error: SDL_Init(): %s\n", SDL_GetError());
        return -1;
    }
    SDL_Log("SDL initialized successfully");
    return 0;
}

int Window::CreateWindow(SDL_WindowFlags window_flags)
{
    this->m_main_scale = SDL_GetDisplayContentScale(SDL_GetPrimaryDisplay());
    SDL_Log("Main scale: %f\n", m_main_scale); 
    m_sdl_window = SDL_CreateWindow(m_window_title, (int)(m_width * m_main_scale), (int)(m_height * m_main_scale), window_flags);
    if (!m_sdl_window)
    {
        SDL_Log("Error: SDL_CreateWindow(): %s\n", SDL_GetError()) ;
        return -1;
    }
    SDL_Log("Window created successfully: %s", m_window_title);
    SDL_SetWindowPosition(m_sdl_window, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED);
    SDL_ShowWindow(m_sdl_window);
    return 0;
}

int Window::CreateGPUDevice()
{
    m_gpu_device = SDL_CreateGPUDevice
    (
        SDL_GPU_SHADERFORMAT_SPIRV |
        SDL_GPU_SHADERFORMAT_DXIL |
        SDL_GPU_SHADERFORMAT_METALLIB,
        true,
        nullptr
    );
    if (!m_gpu_device)
    {
        SDL_Log("Error: SDL_CreateGPUDevice(): %s\n", SDL_GetError());
        return -1;
    }
    SDL_Log("GPU Device created sucessfully");
    return 0;
}

SDL_Window* Window::GetSDLWindow() const
{ 
    return m_sdl_window; 
}

SDL_GPUDevice* Window::GetGPUDevice() const
{
    return m_gpu_device;
}

void Window::SetFullscreen(bool enable)
{
    SDL_Log("Setting fullscreen: %d\n", enable);
    const Uint32 flags = SDL_GetWindowFlags(m_sdl_window);
    const bool is_fullscreen = (flags & SDL_WINDOW_FULLSCREEN) != 0;
    if (enable == is_fullscreen)
        return;
    SDL_Log("Toggling fullscreen mode\n");
    SDL_SetWindowFullscreen(m_sdl_window, enable);
    SDL_SyncWindow(m_sdl_window);
}

float Window::GetMainScale()
{
    return m_main_scale;
}

int Window::ClaimWindowForGPUDevice()
{
    if (!SDL_ClaimWindowForGPUDevice(m_gpu_device, m_sdl_window))
    {
        SDL_Log("Error: SDL_ClaimWindowForGPUDevice(): %s\n", SDL_GetError());
        return -1;
    }
    return 0;
}

int Window::SetGPUSwapchainParams
(
    SDL_GPUSwapchainComposition swapchain_composition, 
    SDL_GPUPresentMode present_mode
)
{
    if(!SDL_SetGPUSwapchainParameters
    (
        m_gpu_device,
        m_sdl_window,
        SDL_GPU_SWAPCHAINCOMPOSITION_SDR,
        SDL_GPU_PRESENTMODE_VSYNC
    ))
    {
        SDL_Log("Error: SDL_setGPUSwapchainParameters(): %s\n", SDL_GetError());
        return -1;
    }
    return 0;
}