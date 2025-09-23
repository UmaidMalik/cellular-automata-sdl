#pragma once

#include <algorithm>
#include <cmath>

namespace math
{
    struct ivec2
    {
        int x;
        int y;
        ivec2(int _x, int _y)
            : x(_x), y(_y) {}
        ivec2 operator+(const ivec2 v) const
        {
            return { y + v.y, y + v.y };
        }
    };

    struct ivec3
    {
        int x;
        int y;
        int z;
        ivec3(int _x, int _y, int _z)
            : x(_x), y(_y), z(_z) {}
        ivec3 operator+(const ivec3 v) const
        {
            return { x + v.x, y + v.y, z + v.z };
        }
    };
    
    struct ivec4
    {
        int x;
        int y;
        int z;
        int a;
        ivec4(int _x, int _y, int _z, int _a)
            : x(_x), y(_y), z(_z), a(_a) {}
        ivec4 operator+(const ivec4 v) const
        {
            return { x + v.x, y + v.y, z + v.z, a + v.a };
        }
    };

    struct vec2
    {
        float x;
        float y;
        vec2(float _x, float _y)
            : x(_x), y(_y) {}
        vec2 operator+(const vec2 v) const
        {
            return { x + v.x, y + v.y };
        }
    };

    struct vec3
    {
        float x;
        float y;
        float z;
        vec3(float _x, float _y, float _z)
            : x(_x), y(_y), z(_z) {}
        vec3 operator+(const vec3 v) const
        {
            return { x + v.x, y + v.y, z + v.z };
        }
    };

    struct vec4
    {
        float x;
        float y;
        float z;
        float a;
        vec4(float _x, float _y, float _z, float _a)
            : x(_x), y(_y), z(_z), a(_a) {}
        vec4 operator+(const vec4 v) const
        {
            return { x + v.x, y + v.y, z + v.z, a + v.a };
        }
    };

    struct RGBAColor
    {
        float r, g, b, a;
        RGBAColor(float _r, float _g, float _b, float _a)
            : r(_r), g(_g), b(_b), a(_a) {}
        RGBAColor operator+(const RGBAColor c) const
        {
            return { r + c.r, g + c.g, b + c.b, a + c.a };
        }
    };

    struct RGBColor
    {
        float r, g, b;
        RGBColor(float _r, float _g, float _b)
            : r(_r), g(_g), b(_b) {}
        RGBColor operator+(const RGBColor c) const
        {
            return { r + c.r, g + c.g, b + c.b };
        }
    };

    struct HSVColor
    {
        float h, s, v;
        HSVColor(float _h, float _s, float _v)
            : h(_h), s(_s), v(_v) {}
        HSVColor operator+(const HSVColor c) const
        {
            return { h + c.h, s + c.s, v + c.v };
        }
    };
    
    RGBColor HSVtoRGB(HSVColor hsv);

    HSVColor RGBtoHSV(RGBColor rgb);

    float fract(float x);
};

math::RGBColor math::HSVtoRGB(HSVColor hsv)
{
    float h = hsv.h, s = hsv.s, v = hsv.v;

  
    if (!std::isfinite(h)) h = 0.0f;
    h = h - std::floor(h); 
    s = std::clamp(s, 0.0f, 1.0f);
    v = std::clamp(v, 0.0f, 1.0f);

    if (s <= 0.0f) return { v, v, v };

    float hh = h * 6.0f;
    int   i  = (int)std::floor(hh);
    float f  = hh - (float)i;

    float p = v * (1.0f - s);
    float q = v * (1.0f - s * f);
    float t = v * (1.0f - s * (1.0f - f));

    switch (i % 6) {
        case 0: return { v, t, p };
        case 1: return { q, v, p };
        case 2: return { p, v, t };
        case 3: return { p, q, v };
        case 4: return { t, p, v };
        default:return { v, p, q };
    }
}

math::HSVColor math::RGBtoHSV(RGBColor rgb)
{
    float r = rgb.r, g = rgb.g, b = rgb.b;
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