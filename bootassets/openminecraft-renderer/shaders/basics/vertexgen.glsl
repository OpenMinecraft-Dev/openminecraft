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

int vertexgen_id() {
#ifdef VULKAN
    return gl_VertexIndex;
#else
    return gl_VertexID;
#endif
}

vec2 vertexgen_quad_normal() {
    return normal[vertexgen_id()];
}

vec2 vertexgen_quad_normal_ccw() {
    return normal[5 - vertexgen_id()];
}

vec2 vertexgen_quad_ndc() {
    return ndc[vertexgen_id()];
}

#endif
