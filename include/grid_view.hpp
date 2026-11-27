#pragma once

#include <SDL3/SDL_rect.h>
#include <algorithm>

struct GridView
{
    static constexpr float MIN_ZOOM = 1.0f;
    static constexpr float MAX_ZOOM = 128.0f;
    static constexpr float DEFAULT_ZOOM = 4.0f;

    float zoom = MIN_ZOOM;
    float left = 0.0f;
    float top = 0.0f;

    void SetZoom(float requested, float anchor_x, float anchor_y)
    {
        const float next_zoom = std::clamp(requested, MIN_ZOOM, MAX_ZOOM);
        const float extent = 1.0f / next_zoom;
        const float difference = 1.0f / zoom - extent;
        left = std::clamp(left + anchor_x * difference, 0.0f, 1.0f - extent);
        top = std::clamp(top + anchor_y * difference, 0.0f, 1.0f - extent);
        zoom = next_zoom;
    }

    void Pan(float dx, float dy)
    {
        const float extent = 1.0f / zoom;
        left = std::clamp(left - dx * extent, 0.0f, 1.0f - extent);
        top = std::clamp(top - dy * extent, 0.0f, 1.0f - extent);
    }

    SDL_Point CellAt(float x, float y, int width, int height) const
    {
        return {
            std::clamp(static_cast<int>((left + x / zoom) * width), 0, width - 1),
            std::clamp(static_cast<int>((top + y / zoom) * height), 0, height - 1)
        };
    }
};
