#pragma once

#include <SDL3/SDL_rect.h>
#include <algorithm>

struct GridAspectRatio
{
    const char* label;
    int width;
    int height;
};

inline constexpr GridAspectRatio GRID_ASPECT_RATIOS[]{
    {"Auto (fit window)", 0, 0},
    {"16:9", 16, 9},
    {"5:4", 5, 4},
    {"4:3", 4, 3},
    {"1:1", 1, 1},
    {"9:16", 9, 16},
    {"Galaxy S23 portrait (6:13)", 6, 13},
    {"Galaxy S23 landscape (13:6)", 13, 6},
};

inline SDL_Rect FitGridViewport(int width, int height, const GridAspectRatio& ratio)
{
    if (ratio.width == 0)
        return {0, 0, width, height};

    const int scale = std::min(width / ratio.width, height / ratio.height);
    const int viewport_width = ratio.width * scale;
    const int viewport_height = ratio.height * scale;
    return {(width - viewport_width) / 2, (height - viewport_height) / 2,
            viewport_width, viewport_height};
}

inline SDL_Point GridSizeForAspect(int long_edge, const GridAspectRatio& ratio, SDL_Point window)
{
    const int width = ratio.width > 0 ? ratio.width : std::max(1, window.x);
    const int height = ratio.height > 0 ? ratio.height : std::max(1, window.y);
    const int longest = std::max(width, height);
    return {
        std::max(1, (long_edge * width + longest / 2) / longest),
        std::max(1, (long_edge * height + longest / 2) / longest)
    };
}

struct CenteredGridCopy
{
    SDL_Rect source;
    SDL_Rect destination;
};

inline CenteredGridCopy GridOverlap(int old_width, int old_height, int new_width, int new_height)
{
    const int width = std::min(old_width, new_width);
    const int height = std::min(old_height, new_height);
    return {
        {old_width / 2 - width / 2, old_height / 2 - height / 2, width, height},
        {new_width / 2 - width / 2, new_height / 2 - height / 2, width, height}
    };
}

struct GridView
{
    static constexpr float MIN_ZOOM = 1.0f;
    static constexpr float MAX_ZOOM = 128.0f;
    static constexpr float DEFAULT_ZOOM = 4.0f;

    float zoom = MIN_ZOOM;
    float left = 0.0f;
    float top = 0.0f;
    int aspect_index = 0;

    float ExtentX() const { return width_scale / zoom; }
    float ExtentY() const { return height_scale / zoom; }

    bool SetViewport(const SDL_Rect& viewport, int grid_width, int grid_height)
    {
        if (viewport.w <= 0 || viewport.h <= 0)
            return false;

        const float aspect = (static_cast<float>(viewport.w) * grid_height) /
                             (static_cast<float>(viewport.h) * grid_width);
        const float next_width = std::min(aspect, 1.0f);
        const float next_height = std::min(1.0f / aspect, 1.0f);
        if (next_width == width_scale && next_height == height_scale)
            return false;

        const float center_x = left + ExtentX() * 0.5f;
        const float center_y = top + ExtentY() * 0.5f;
        width_scale = next_width;
        height_scale = next_height;
        left = std::clamp(center_x - ExtentX() * 0.5f, 0.0f, 1.0f - ExtentX());
        top = std::clamp(center_y - ExtentY() * 0.5f, 0.0f, 1.0f - ExtentY());
        return true;
    }

    void SetZoom(float requested, float anchor_x, float anchor_y)
    {
        const float old_width = ExtentX();
        const float old_height = ExtentY();
        zoom = std::clamp(requested, MIN_ZOOM, MAX_ZOOM);
        left = std::clamp(left + anchor_x * (old_width - ExtentX()), 0.0f, 1.0f - ExtentX());
        top = std::clamp(top + anchor_y * (old_height - ExtentY()), 0.0f, 1.0f - ExtentY());
    }

    void Pan(float dx, float dy)
    {
        left = std::clamp(left - dx * ExtentX(), 0.0f, 1.0f - ExtentX());
        top = std::clamp(top - dy * ExtentY(), 0.0f, 1.0f - ExtentY());
    }

    SDL_Point CellAt(float x, float y, int width, int height) const
    {
        return {
            std::clamp(static_cast<int>((left + x * ExtentX()) * width), 0, width - 1),
            std::clamp(static_cast<int>((top + y * ExtentY()) * height), 0, height - 1)
        };
    }

private:
    float width_scale = 1.0f;
    float height_scale = 1.0f;
};
