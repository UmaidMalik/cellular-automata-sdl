#version 450
#ifdef GL_ES
#define VERTEX_INDEX gl_VertexID
#else
#define VERTEX_INDEX gl_VertexIndex
#endif

#ifdef GL_ES
out vec2 vUV;
#else
layout (location = 0) out vec2 vUV;
#endif

void main()
{
    const vec2 pos[3] = vec2[3]( vec2(-1.0,-1.0), vec2(3.0,-1.0), vec2(-1.0,3.0) );
    const vec2 uv[3] = vec2[3]( vec2(0.0,0.0), vec2(2.0,0.0), vec2(0.0,2.0) );
    gl_Position = vec4(pos[VERTEX_INDEX], 0.0, 1.0);
    vUV = uv[VERTEX_INDEX];
}