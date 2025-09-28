#version 450
layout (location = 0) out vec4 outState;
layout(set = 2, binding = 0) uniform sampler2D uCurrent;
layout(set = 3, binding = 0) uniform Params
{
    ivec2 grid; // W, H
    int range; // >= 1
    int wrap; // 0 or 1
    int max_states; // >= 2
    uvec2 born_mask;
    uvec2 survive_mask;
} U;

bool bit(uvec2 m, int i)
{
    return (i < 32) ? (((m.x >> uint(i)) & 1u) != 0u)
                    : (((m.y >> uint(i-32)) & 1u) != 0u);
}

int wrap_or_clamp(int v, int hi)
{
    if (U.wrap == 1)
    {
        v = (v % hi + hi) % hi;
    }
    else
    {
        v = clamp(v, 0, hi - 1);
    }
    return v;
}

int read_state(ivec2 p)
{
    float r = texelFetch(uCurrent, p, 0).r;
    int s_max = max(U.max_states - 1, 1);
    int s = int(round(r * float(s_max)));
    return clamp(s, 0, s_max);
}

void main()
{
    ivec2 p = ivec2(gl_FragCoord.xy);

    int n = 0;
    for (int dy = -U.range; dy <= U.range; ++dy)
    {
        for (int dx = -U.range; dx <= U.range; ++dx)
        {
            if (dx == 0 && dy == 0) continue;
            ivec2 q = ivec2
            (
                wrap_or_clamp(p.x + dx, U.grid.x),
                wrap_or_clamp(p.y + dy, U.grid.y)
            );
            n += (read_state(q) == (U.max_states - 1)) ? 1 : 0;
        }
    }

    int s = read_state(p);
    int s_max = max(U.max_states - 1, 1);
    int next;

    if (s == 0)
    {
        next = bit(U.born_mask, n) ? s_max : 0;
    }
    else if (s == s_max)
    {
        next = bit(U.survive_mask, n) ? s_max : (s - 1);
    }
    else
    {
        next = s - 1;
    }

    float r = float(next) / float(s_max);
    outState = vec4(r, 0.0, 0.0, 1.0);
}