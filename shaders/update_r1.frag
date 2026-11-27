#version 450
layout(location = 0) out vec4 outState;
layout(set = 2, binding = 0) uniform sampler2D uCurrent;
layout(std140, set = 3, binding = 0) uniform Params
{
    ivec4 grid; // width, height, range, wrap
    int agingStages;
    float decay;
    uvec4 rules; // birth low/high, survival low/high
} U;

bool matchesRule(uint lo, uint hi, int count)
{
    return count < 32 ? (lo & (1u << uint(count))) != 0u
                      : (hi & (1u << uint(count - 32))) != 0u;
}

ivec2 samplePosition(ivec2 position)
{
    ivec2 size = U.grid.xy;
    return U.grid.w != 0 ? (position % size + size) % size
                         : clamp(position, ivec2(0), size - 1);
}

// Alpha encodes dead as 0, alive as 1, and aging as remaining stages / 256.
const float STATE_SCALE = 256.0;
const int LIVE_STATE = 256;

void main()
{
    ivec2 position = ivec2(gl_FragCoord.xy);
    int neighbors = 0;
    float hueSum = 0.0;
    for (int dy = -U.grid.z; dy <= U.grid.z; ++dy)
    {
        for (int dx = -U.grid.z; dx <= U.grid.z; ++dx)
        {
            if (dx == 0 && dy == 0)
                continue;
            vec4 neighbor = texelFetch(uCurrent, samplePosition(position + ivec2(dx, dy)), 0);
            if (neighbor.a == 1.0)
            {
                ++neighbors;
                hueSum += neighbor.r;
            }
        }
    }

    vec4 cell = texelFetch(uCurrent, position, 0);
    int state = int(round(cell.a * STATE_SCALE));
    if (state != LIVE_STATE && U.agingStages == 0)
        state = 0;
    bool born = state == 0 && matchesRule(U.rules.x, U.rules.y, neighbors);
    bool survives = state == LIVE_STATE && matchesRule(U.rules.z, U.rules.w, neighbors);
    if (state == 0 && !born)
    {
        // Round down to prevent small decay steps from getting stuck at a nonzero UNORM value.
        float value = floor(round(cell.b * 65535.0) * U.decay) / 65535.0;
        outState = vec4(cell.r, 1.0, value, 0.0);
        return;
    }

    int nextState = born || survives ? LIVE_STATE : min(U.agingStages, state - 1);
    float hue = neighbors > 0 ? hueSum / float(neighbors) : 0.0;
    outState = vec4(hue, 1.0, 1.0, float(nextState) / STATE_SCALE);
}
