#version 450

layout (location = 0) out vec2 vUV;
layout (set=1, binding=0) uniform PresentBox { vec4 box; };

void main()
{
    const vec2 pos[3] = vec2[3]( vec2(-1.0,-1.0), vec2(3.0,-1.0), vec2(-1.0,3.0) );
    const vec2 uv[3] = vec2[3]( vec2(0.0,0.0), vec2(2.0,0.0), vec2(0.0,2.0) );
    vec2 uv01 = uv[gl_VertexIndex] * 1.0;
    vUV = uv01;
    vec2 ndc = mix(box.xy, box.zw, uv01);
    gl_Position = vec4(pos[gl_VertexIndex], 0.0, 1.0);
    
}
