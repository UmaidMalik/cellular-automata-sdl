#version 450
layout (location = 0) out vec4 outState;

layout(set = 2, binding = 0) uniform sampler2D uCurrent;
//ParamsUBO params{ W, H, auto_sim.GetRange(), 1, auto_sim.GetMaxStates() };
layout(set = 3, binding = 0) uniform Params
{
    ivec2 grid; // W, H
    int range; // >= 1
    int wrap; // 0 or 1
    int max_states; // >= 2
    uvec2 born_mask;
    uvec2 survive_mask;
} U;

int alive_at(ivec2 p)
{
    if (U.wrap == 1)
    {
        p = ivec2
        (
            (p.x % U.grid.x + U.grid.x) % U.grid.x,
            (p.y % U.grid.y + U.grid.y) % U.grid.y
        );
    }
    else
    {
        p = clamp(p, ivec2(0), U.grid - 1);
    }
    return texelFetch(uCurrent, p, 0).r > 0.5 ? 1: 0;
}

void main()
{
    ivec2 p = ivec2(gl_FragCoord.xy);

    int s = alive_at(p);
    int n = 0;
    for (int dy=-U.range; dy<=U.range; ++dy)
    {
        for (int dx=-U.range; dx<=U.range; ++dx)
        {
            if (dx==0 && dy==0) continue;
            n += alive_at(p + ivec2(dx,dy));
        }
    }
    int next = ((s == 1 && (n == 0 || n == 1 || n == 2 || n == 3 || n == 4 || n == 5 || n == 6 || n == 7 || n == 8)) || (s == 0 && n == 3)) ? 1 : 0;
    outState = vec4(float(next), 0.0, 0.0, 1.0);
}