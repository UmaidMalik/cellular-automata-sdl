#pragma once

#include <algorithm>

namespace math
{
    struct ivec2
    {
        int x;
        int y;
        ivec2(int _x, int _y)
            : x(_x), y(_y) {}
    };

    struct ivec3
    {
        int x;
        int y;
        int z;
        ivec3(int _x, int _y, int _z)
            : x(_x), y(_y), z(_z) {}
    };
    
    struct ivec4
    {
        int x;
        int y;
        int z;
        int a;
        ivec4(int _x, int _y, int _z, int _a)
            : x(_x), y(_y), z(_z), a(_a) {}
    };

    struct vec2
    {
        float x;
        float y;
        vec2(float _x, float _y)
            : x(_x), y(_y) {}
    };

    struct vec3
    {
        float x;
        float y;
        float z;
        vec3(float _x, float _y, float _z)
            : x(_x), y(_y), z(_z) {}
    };

    struct vec4
    {
        float x;
        float y;
        float z;
        float a;
        vec4(float _x, float _y, float _z, float _a)
            : x(_x), y(_y), z(_z), a(_a) {}
    };

    struct RGBAColor
    {
        float r, g, b, a;
        RGBAColor(float _r, float _g, float _b, float _a)
            : r(_r), g(_g), b(_b), a(_a) {}
    };

    struct RGBColor
    {
        float r, g, b;
        RGBColor(float _r, float _g, float _b)
            : r(_r), g(_g), b(_b) {}
    };

    struct HSVColor
    {
        float h, s, v;
        HSVColor(float _h, float _s, float _v)
            : h(_h), s(_s), v(_v) {}
    };
    
    RGBColor HSVtoRGB(HSVColor hsv_color);

    HSVColor RGBtoHSV(RGBColor rgb_color);

    float fract(float x);
};

math::RGBColor math::HSVtoRGB(math::HSVColor hsv_color)
{
    float h = hsv_color.h, s = hsv_color.s, v = hsv_color.v;
    
    float kR = 5.0f, kG = 3.0f, kB = 1.0f;

    auto f = [](float n, float h)
    {
        float x = math::fract(h + n);
        float t = x * 6.0f;
        return std::clamp(std::min(t - 3.0f, 4.0f - t), 0.0f, 1.0f);
    };

    float r = v * (1.0f - s * (1.0f - f(kR, h)));
    float g = v * (1.0f - s * (1.0f - f(kG, h)));
    float b = v * (1.0f - s * (1.0f - f(kB, h)));

    return math::RGBColor{ r, g, b };
}

math::HSVColor math::RGBtoHSV(math::RGBColor rgb_color)
{
    float r = rgb_color.r, g = rgb_color.g, b = rgb_color.b;
    float c_max = std::max({ r, g, b });
    float c_min = std::min({ r, g, b });
    float delta = c_max - c_min;

    float h = 0.0f, s = 0.0f, v = c_max;

    if (delta > 0.0f)
    {
        if (c_max == r)
        {
            h = (g - b) / delta;
        }
        else if (c_max == g)
        {
            h = (b - r) / delta + 2.0f;
        }
        else if (c_max == b)
        {
            h = (r - g) / delta + 4.0f;
        }

        h /= 6.0f;
        if (h < 0.0f)
        {
            h += 1.0f;
        }

        s = delta / c_max;
    }
    return math::HSVColor{h, s, v};
}

float math::fract(float x)
{
    return x - std::floor(x);
}