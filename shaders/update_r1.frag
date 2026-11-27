#version 450
#ifdef GL_ES
layout(location = 0) out highp uvec4 outState;
uniform highp usampler2D uCurrent;
layout(std140) uniform Params
#else
layout(location = 0) out vec4 outState;
layout(set = 2, binding = 0) uniform sampler2D uCurrent;
layout(std140, set = 3, binding = 0) uniform Params
#endif
{
    ivec4 grid; // width, height, range, wrap
    int agingStages;
    float decay;
    int neighborhoodShape;
    uvec4 birthMask;
    uvec4 survivalMask;
} U;

bool matchesRule(uvec4 mask, int count)
{
    return (mask[count / 32] & (1u << uint(count % 32))) != 0u;
}

ivec2 samplePosition(ivec2 position)
{
    ivec2 size = U.grid.xy;
    return U.grid.w != 0 ? (position % size + size) % size
                         : clamp(position, ivec2(0), size - 1);
}

// RG stores hue direction, B stores brightness, and alpha packs cell state
// (0 dead, 1–255 aging, 256 alive) above seven bits of saturation percent.
const uint SATURATION_BITS = 7u;
const uint SATURATION_MASK = (1u << SATURATION_BITS) - 1u;
const int LIVE_STATE = 256;
const float UNORM_MAX = 65535.0;
const float LIVE_ALPHA = float(uint(LIVE_STATE) << SATURATION_BITS) / UNORM_MAX;

vec4 readCell(ivec2 position)
{
#ifdef GL_ES
    return vec4(texelFetch(uCurrent, position, 0)) / UNORM_MAX;
#else
    return texelFetch(uCurrent, position, 0);
#endif
}

void writeCell(vec4 cell)
{
#ifdef GL_ES
    outState = uvec4(round(clamp(cell, 0.0, 1.0) * UNORM_MAX));
#else
    outState = cell;
#endif
}

uint readPackedState(float alpha)
{
    return uint(round(alpha * UNORM_MAX));
}

float packState(int state, uint saturation)
{
    return float((uint(state) << SATURATION_BITS) | saturation) / UNORM_MAX;
}

void main()
{
    ivec2 position = ivec2(gl_FragCoord.xy);
    int neighbors = 0;
    uint saturationSum = 0u;
    // RG stores a unit hue vector mapped from [-1, 1] to UNORM.
    vec2 hueSum = vec2(0.0);
    vec2 firstHue = vec2(1.0, 0.5); // Red if a B0 birth has no neighbors.
    for (int dy = -U.grid.z; dy <= U.grid.z; ++dy)
    {
        int radius = U.grid.z;
        int firstX = -radius;
        int lastX = radius;
        if (U.neighborhoodShape == 1) // Von Neumann
        {
            lastX = radius - abs(dy);
            firstX = -lastX;
        }
        else if (U.neighborhoodShape == 2) // Circular, measured between cell centers
        {
            lastX = int(sqrt(float(radius * radius - dy * dy)) + 0.001);
            firstX = -lastX;
        }
        else if (U.neighborhoodShape == 3) // Axial hex coordinates on a square texture
        {
            firstX = max(-radius, dy - radius);
            lastX = min(radius, dy + radius);
        }
        for (int dx = firstX; dx <= lastX; ++dx)
        {
            if (dx == 0 && dy == 0)
                continue;
            vec4 neighbor = readCell(samplePosition(position + ivec2(dx, dy)));
            if (neighbor.a >= LIVE_ALPHA)
            {
                if (neighbors == 0)
                    firstHue = neighbor.rg;
                ++neighbors;
                saturationSum += readPackedState(neighbor.a) & SATURATION_MASK;
                hueSum += neighbor.rg * 2.0 - 1.0;
            }
        }
    }

    vec4 cell = readCell(position);
    uint packedState = readPackedState(cell.a);
    int state = int(packedState >> SATURATION_BITS);
    uint saturation = packedState & SATURATION_MASK;
    if (state != LIVE_STATE && U.agingStages == 0)
        state = 0;
    bool born = state == 0 && matchesRule(U.birthMask, neighbors);
    bool survives = state == LIVE_STATE && matchesRule(U.survivalMask, neighbors);
    if (state == 0 && !born)
    {
        // Round down to prevent small decay steps from getting stuck at a nonzero UNORM value.
        float value = floor(round(cell.b * 65535.0) * U.decay) / 65535.0;
        writeCell(vec4(cell.rg, value, packState(0, saturation)));
        return;
    }

    if (born || survives)
    {
        // Opposing hues can cancel. Keep a survivor's hue, or inherit the
        // first live neighbor's hue for a birth with no clear average.
        vec2 hue = survives ? cell.rg : firstHue;
        float lengthSquared = dot(hueSum, hueSum);
        float quantizationError = 2.0 * float(neighbors) / 65535.0;
        if (lengthSquared > quantizationError * quantizationError)
            hue = hueSum * inversesqrt(lengthSquared) * 0.5 + 0.5;
        if (born)
        {
            uint count = uint(neighbors);
            saturation = count == 0u ? 100u : (saturationSum + count / 2u) / count;
        }
        writeCell(vec4(hue, 1.0, packState(LIVE_STATE, saturation)));
        return;
    }

    int nextState = min(U.agingStages, state - 1);
    writeCell(vec4(cell.rgb, packState(nextState, saturation)));
}
