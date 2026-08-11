#ifndef VERTEXGEN_GLSL
#define VERTEXGEN_GLSL

const vec2 ndc[6] = vec2[6](
    vec2(-1.0, -1.0),
    vec2(-1.0, 1.0),
    vec2(1.0, 1.0),
    vec2(1.0, 1.0),
    vec2(1.0, -1.0),
    vec2(-1.0, -1.0)
);

const vec2 normal[6] = vec2[6](
    vec2(0.0, 0.0),
    vec2(0.0, 1.0),
    vec2(1.0, 1.0),
    vec2(1.0, 1.0),
    vec2(1.0, 0.0),
    vec2(0.0, 0.0)
);

vec2 vertexgen_quad_normal() {
#ifdef VULKAN
    int vid = gl_VertexIndex;
#else
    int vid = gl_VertexID;
#endif

    return normal[vid];
}

vec2 vertexgen_quad_ndc() {
#ifdef VULKAN
    int vid = gl_VertexIndex;
#else
    int vid = gl_VertexID;
#endif

    return ndc[vid];
}

int vertexgen_id() {
#ifdef VULKAN
    return gl_VertexIndex;
#else
    return gl_VertexID;
#endif
}

#endif
