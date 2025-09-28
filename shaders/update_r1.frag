#version 450
layout (location = 0) out vec4 outState;
layout(set = 2, binding = 0) uniform sampler2D uCurrent;
layout(std140, set = 3, binding = 0) uniform Params
{
    ivec4 lane0;    // .x=W, .y=H, .z=range, .w=wrap
    ivec4 lane1;    // .x=max_states
    uvec4 lane2;    // .x=born_lo, .y=born_hi, .z=surv_lo, .w=surv_hi
} U;

#define W           (U.lane0.x)
#define H           (U.lane0.y)
#define RANGE       (U.lane0.z)
#define WRAP        (U.lane0.w)
#define MAX_STATE   (U.lane1.x)
#define BORN_LO     (U.lane2.x)
#define BORN_HI     (U.lane2.y)
#define SURV_LO     (U.lane2.z)
#define SURV_HI     (U.lane2.w)

bool bit64(uint lo, uint hi, int i)
{
    return (i < 32) ? (((lo >> uint(i)) & 1u) != 0u)
                    : (((hi >> uint(i-32)) & 1u) != 0u);
}

int wrap_or_clamp(int v, int hi)
{
    return (WRAP == 1) ? ( (v % hi + hi) % hi ) : clamp(v, 0, hi - 1);
}

int read_state(ivec2 p)
{
    float r = texelFetch(uCurrent, p, 0).r;
    int s_max = max(MAX_STATE - 1, 1);
    int s = int(round(r * float(s_max)));
    return clamp(s, 0, s_max);
}

void main()
{
    ivec2 p = ivec2(gl_FragCoord.xy);

    int alive_top = max(MAX_STATE - 1, 1);
    int n = 0;
    for (int dy = -RANGE; dy <= RANGE; ++dy)
    {
        for (int dx = -RANGE; dx <= RANGE; ++dx)
        {
            if (dx == 0 && dy == 0) continue;
            ivec2 q = ivec2
            (
                wrap_or_clamp(p.x + dx, W),
                wrap_or_clamp(p.y + dy, H)
            );
            n += (read_state(q) == alive_top) ? 1 : 0;
        }
    }

    int s = read_state(p);
    int next;
    if (s == 0)
    {
        next = bit64(BORN_LO, BORN_HI, n) ? alive_top : 0;
    }
    else if (s == alive_top)
    {
        next = bit64(SURV_LO, SURV_HI, n) ? alive_top : (s - 1);
    }
    else
    {
        next = s - 1;
    }

    float r = float(next) / float(alive_top);
    outState = vec4(r, 0.0, 0.0, 1.0);
}