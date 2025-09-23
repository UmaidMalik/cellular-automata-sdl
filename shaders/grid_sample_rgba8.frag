/*
#version 450
layout(location=0) out vec4 outColor;
layout(set=0, binding=0) uniform sampler2D uTex;
void main() {
    // sample texel (0,0) directly
    outColor = texelFetch(uTex, ivec2(0,0), 0);
}
*/

#version 450

layout (location = 0) in vec2 vUV;
layout (location = 0) out vec4 outColor;
layout (set = 2, binding = 0) uniform sampler2D uTex;

void main()
{
    outColor = texture(uTex, vUV);
}


/*
#version 450
layout(location=0) out vec4 outColor;
void main() { outColor = vec4(1,0,1,1); }
*/