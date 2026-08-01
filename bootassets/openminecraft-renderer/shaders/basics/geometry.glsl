#ifndef GEOMETRY_GLSL
#define GEOMETRY_GLSL

int selectQuadrant(vec2 p) {
    ivec2 mask = ivec2(step(0.0, p));
    return mask.y << 1 | ((1 - mask.x) ^ mask.y);
}

vec2 rectCenter(vec4 pos) {
    return pos.xy + pos.zw / 2;
}

#endif
