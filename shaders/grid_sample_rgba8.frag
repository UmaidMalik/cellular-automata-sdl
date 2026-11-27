#version 450
#ifdef GL_ES
in vec2 vUV;
#else
layout(location = 0) in vec2 vUV;
#endif
layout(location = 0) out vec4 outColor;
#ifdef GL_ES
uniform highp usampler2D uTex;
layout(std140) uniform View
#else
layout(set = 2, binding = 0) uniform sampler2D uTex;
layout(std140, set = 3, binding = 0) uniform View
#endif
{
    vec4 view; // left, top, extent
} U;

void main()
{
    vec2 screen = vec2(vUV.x, 1.0 - vUV.y);
#ifdef GL_ES
    vec4 color = vec4(texture(uTex, U.view.xy + screen * U.view.z)) / 65535.0;
#else
    vec4 color = texture(uTex, U.view.xy + screen * U.view.z);
#endif
    if (color.b == 0.0)
    {
        outColor = vec4(0.0, 0.0, 0.0, 1.0);
        return;
    }

    vec2 direction = color.rg * 2.0 - 1.0;
    float hue = atan(direction.y, direction.x) / 6.28318530718;
    vec3 rgb = abs(fract(hue + vec3(0.0, 2.0 / 3.0, 1.0 / 3.0)) * 6.0 - 3.0);
    vec3 saturated = clamp(rgb - 1.0, 0.0, 1.0);
    // The update shader packs saturation percent into alpha's seven low bits.
    float saturation = float(uint(round(color.a * 65535.0)) & 127u) / 100.0;
    outColor = vec4(color.b * mix(vec3(1.0), saturated, saturation), 1.0);
}
