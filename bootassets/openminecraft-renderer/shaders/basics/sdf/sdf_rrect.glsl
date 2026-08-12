#ifndef SDF_RRECT_GLSL
#define SDF_RRECT_GLSL

#include "basics/geometry.glsl"

float sdf_rrect(vec2 p, vec2 halfSize, vec4 radii, float thickness, float smoothEdge)
{
    float radius = radii[geom_selectQuadrant(p)];
    radius = min(radius, min(halfSize.x, halfSize.y));
    vec2 centerDis = abs(p) - halfSize + radius;
    float edgeDis = length(max(centerDis, 0.0)) + min(max(centerDis.x, centerDis.y), 0.0) - radius;
    return 1.0 - smoothstep(thickness, thickness + smoothEdge, edgeDis);
}

#endif
