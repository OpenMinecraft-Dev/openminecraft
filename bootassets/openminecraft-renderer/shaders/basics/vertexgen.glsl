#ifndef VERTEXGEN_GLSL
#define VERTEXGEN_GLSL

const vec2 ndc[6] =
    vec2[6](vec2(-1.0, -1.0), vec2(-1.0, 1.0), vec2(1.0, 1.0), vec2(1.0, 1.0), vec2(1.0, -1.0), vec2(-1.0, -1.0));

const vec2 normal[6] =
    vec2[6](vec2(0.0, 0.0), vec2(0.0, 1.0), vec2(1.0, 1.0), vec2(1.0, 1.0), vec2(1.0, 0.0), vec2(0.0, 0.0));

// 4-vertex quad (with index buffer 0,1,2,2,3,0):
// Produces exactly the same two triangles as the 6-vertex version above, but runs the vertex shader only 4 times.
const vec2 normal4[4] = vec2[4](vec2(0.0, 0.0), vec2(0.0, 1.0), vec2(1.0, 1.0), vec2(1.0, 0.0));
const vec2 normal4ccw[4] = vec2[4](vec2(0.0, 0.0), vec2(1.0, 0.0), vec2(1.0, 1.0), vec2(0.0, 1.0));

int vertexgen_instanceid()
{
#ifdef VULKAN
    return gl_InstanceIndex;
#else
    return gl_InstanceID;
#endif
}

int vertexgen_id()
{
#ifdef VULKAN
    return gl_VertexIndex;
#else
    return gl_VertexID;
#endif
}

vec2 vertexgen_quad_normal()
{
    return normal[vertexgen_id()];
}

vec2 vertexgen_quad_normal_ccw()
{
    return normal[5 - vertexgen_id()];
}

vec2 vertexgen_quad_normal4()
{
    return normal4[vertexgen_id()];
}

vec2 vertexgen_quad_normal4_ccw()
{
    return normal4ccw[vertexgen_id()];
}

vec2 vertexgen_quad_ndc()
{
    return ndc[vertexgen_id()];
}

#endif
