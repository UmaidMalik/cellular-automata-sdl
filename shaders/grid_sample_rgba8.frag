#version 450
layout(location = 0) in vec2 vUV;
layout(location = 0) out vec4 outColor;
layout(set = 2, binding = 0) uniform sampler2D uTex;
layout(std140, set = 3, binding = 0) uniform View
{
    vec4 viewport; // left, top, extent
} U;

void main()
{
    vec2 screen = vec2(vUV.x, 1.0 - vUV.y);
    vec3 hsv = texture(uTex, U.viewport.xy + screen * U.viewport.z).rgb;
    vec3 hue = abs(fract(hsv.x + vec3(0.0, 2.0 / 3.0, 1.0 / 3.0)) * 6.0 - 3.0);
    vec3 rgb = hsv.z * mix(vec3(1.0), clamp(hue - 1.0, 0.0, 1.0), hsv.y);
    outColor = vec4(rgb, 1.0);
}
