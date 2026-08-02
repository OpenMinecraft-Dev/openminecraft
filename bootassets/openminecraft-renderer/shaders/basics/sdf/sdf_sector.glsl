#ifndef SDF_SECTOR_GLSL
#define SDF_SECTOR_GLSL

#include "basics/geometry.glsl"

float sdf_sector(vec2 p, vec2 halfSize, vec2 sectorCenter, vec2 angles, float thickness, float smoothEdge) {
    float weight = min(halfSize.x, halfSize.y) - distance(sectorCenter, p);
    vec2 n1 = vec2(-sin(angles.x), -cos(angles.x));
    vec2 n2 = vec2(sin(angles.y), cos(angles.y));
    
    float d1 = dot(p - sectorCenter, n1);
    float d2 = dot(p - sectorCenter, n2);

    return smoothstep(thickness, thickness + smoothEdge, min(weight, min(d1, d2)));
}

#endif
