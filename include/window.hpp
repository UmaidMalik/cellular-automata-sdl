#pragma once

#include <SDL3/SDL.h>

class Window
{
    private:
        int m_width;
        int m_height;
        const char* m_window_title;
        SDL_Window* m_sdl_window = nullptr;
        SDL_GPUDevice* m_gpu_device = nullptr;
        bool m_window_claimed = false;
        float m_main_scale;
        bool m_running = false;
    public:
        Window(int width, int height, const char* title);
        ~Window();
        int InitWindow();
        int CreateWindow(SDL_WindowFlags window_flags);
        int CreateGPUDevice();
        SDL_Window* GetSDLWindow() const;
        SDL_GPUDevice* GetGPUDevice() const;
        int ClaimWindowForGPUDevice();
        bool SetupWindow();
        int SetGPUSwapchainParams
        (
            SDL_GPUSwapchainComposition swapchain_composition,
            SDL_GPUPresentMode present_mode
        );
        void SetFullscreen(bool enable);
        float GetMainScale();
        void StartRun();
        void EndRun();
        bool IsRunning();
};