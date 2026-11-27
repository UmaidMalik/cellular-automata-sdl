#include "window.hpp"
#include <SDL3/SDL.h>
#include <iostream>
#ifdef __EMSCRIPTEN__
#include <emscripten/html5.h>
#endif

Window::Window(int width, int height, const char* title)
    : m_width(width), m_height(height), m_window_title(title)
{
    std::cout << "Window created\n";
}

Window::~Window()
{
#ifdef __EMSCRIPTEN__
    if (m_gl_context)
        SDL_GL_DestroyContext(m_gl_context);
#else
    if (m_gpu_device)
    {
        SDL_WaitForGPUIdle(m_gpu_device);
        if (m_window_claimed)
            SDL_ReleaseWindowFromGPUDevice(m_gpu_device, m_sdl_window);
        SDL_DestroyGPUDevice(m_gpu_device);
    }
#endif
    if (m_sdl_window)
        SDL_DestroyWindow(m_sdl_window);
    SDL_Quit();

    SDL_Log("~Window() destructor called and exited\n");
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
    return 0;
}

#ifndef __EMSCRIPTEN__
int Window::CreateGPUDevice()
{
    m_gpu_device = SDL_CreateGPUDevice
    (
        SDL_GPU_SHADERFORMAT_SPIRV,
        false,
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

#endif

SDL_Window* Window::GetSDLWindow() const
{ 
    return m_sdl_window; 
}

#ifdef __EMSCRIPTEN__
SDL_GLContext Window::GetGLContext() const
{
    return m_gl_context;
}

void Window::SyncBrowserCanvas()
{
    if (!EM_ASM_INT({
        const pending = Module['canvasResizePending'];
        Module['canvasResizePending'] = false;
        return pending ? 1 : 0;
    }))
        return;

    double css_width = 0.0;
    double css_height = 0.0;
    if (emscripten_get_element_css_size("#canvas", &css_width, &css_height) != EMSCRIPTEN_RESULT_SUCCESS)
        return;

    const int width = SDL_lround(css_width);
    const int height = SDL_lround(css_height);
    if (width <= 0 || height <= 0)
        return;

    int window_width = 0;
    int window_height = 0;
    SDL_GetWindowSize(m_sdl_window, &window_width, &window_height);
    int canvas_width = 0;
    int canvas_height = 0;
    emscripten_get_canvas_element_size("#canvas", &canvas_width, &canvas_height);
    const double pixel_ratio = emscripten_get_device_pixel_ratio();

    // SDL's browser resize callback skips fullscreen. Apply the observed
    // canvas size explicitly so SDL input and the WebGL backing agree.
    if (window_width != width || window_height != height ||
        canvas_width != SDL_lround(width * pixel_ratio) ||
        canvas_height != SDL_lround(height * pixel_ratio))
    {
        SDL_SetWindowSize(m_sdl_window, width, height);
    }
}
#else
SDL_GPUDevice* Window::GetGPUDevice() const
{
    return m_gpu_device;
}

#endif

void Window::SetFullscreen(bool enable)
{
    const Uint32 flags = SDL_GetWindowFlags(m_sdl_window);
    const bool is_fullscreen = (flags & SDL_WINDOW_FULLSCREEN) != 0;
    if (enable == is_fullscreen)
        return;
    SDL_Log("Setting fullscreen: %d", enable);
    SDL_SetWindowFullscreen(m_sdl_window, enable);
    SDL_SyncWindow(m_sdl_window);
}

float Window::GetMainScale()
{
    return m_main_scale;
}

#ifndef __EMSCRIPTEN__
int Window::ClaimWindowForGPUDevice()
{
    if (!SDL_ClaimWindowForGPUDevice(m_gpu_device, m_sdl_window))
    {
        SDL_Log("Error: SDL_ClaimWindowForGPUDevice(): %s\n", SDL_GetError());
        return -1;
    }
    m_window_claimed = true;
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
        swapchain_composition,
        present_mode
    ))
    {
        SDL_Log("Error: SDL_setGPUSwapchainParameters(): %s\n", SDL_GetError());
        return -1;
    }
    return 0;
}

#endif

bool Window::SetupWindow()
{
#ifdef __EMSCRIPTEN__
    if (InitWindow() != 0)
        return false;
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
    if (CreateWindow(SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE | SDL_WINDOW_HIGH_PIXEL_DENSITY) != 0)
        return false;
    m_gl_context = SDL_GL_CreateContext(m_sdl_window);
    if (!m_gl_context)
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Creating WebGL 2 context: %s", SDL_GetError());
    return m_gl_context != nullptr;
#else
    if (InitWindow() != 0 ||
        CreateWindow(SDL_WINDOW_RESIZABLE | SDL_WINDOW_HIDDEN | SDL_WINDOW_HIGH_PIXEL_DENSITY) != 0 ||
        CreateGPUDevice() != 0 ||
        ClaimWindowForGPUDevice() != 0 ||
        SetGPUSwapchainParams(SDL_GPU_SWAPCHAINCOMPOSITION_SDR, SDL_GPU_PRESENTMODE_VSYNC) != 0)
    {
        std::cerr << "Error: window setup failed\n";
        SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Error", "Ran into an error during window setup :(", nullptr);
        return false;
    }
    return true;
#endif
}

void Window::StartRun()
{
    m_running = true;
}

void Window::EndRun()
{
    m_running = false;
}

bool Window::IsRunning()
{
    return m_running;
}

