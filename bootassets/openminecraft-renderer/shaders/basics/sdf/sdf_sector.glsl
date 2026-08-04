#ifndef SDF_SECTOR_GLSL
#define SDF_SECTOR_GLSL

#include "basics/geometry.glsl"

float sdf_sector(vec2 p, vec2 halfSize, vec2 sectorCenter, vec2 angles, float thickness, float smoothEdge) {
    float radius = min(halfSize.x, halfSize.y);
    p -= sectorCenter;
    const float PI = 3.14159265359;
    const float TAU = 2.0 * PI;

    float r = length(p);
    if (r < 0.0001) {
        return 1.0;
    }

    float theta = atan(-p.y, p.x);
    theta = mod(theta, TAU);

    float b = mod(angles.x, TAU);
    float e = mod(angles.y, TAU);
    if (e < b) e += TAU;
    float t = theta;
    if (t < b) t += TAU;

    bool inside = (t >= b && t <= e);

    float angleDist;
    if (inside) {
        float angDiff = min(t - b, e - t);
        angleDist = -r * angDiff;
    } else {
        float d1 = abs(t - b);
        float d2 = abs(t - e);
        float angDiff = min(d1, d2);
        angleDist = r * angDiff;
    }

    float circleDist = r - radius;

    float d = max(circleDist, angleDist);

    return 1.0 - smoothstep(thickness, thickness + smoothEdge, d);
}

#endif
