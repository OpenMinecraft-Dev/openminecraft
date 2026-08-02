#ifndef SDF_TEXT_GLSL
#define SDF_TEXT_GLSL

const float INF = 1.0 / 0.0;
const float EPSILON = 1e-6;
const float DERIVATIVE_THRESHOLD = 1e-12;
const int ITERATION = 3;

float sdf_distanceToLineSegment(vec2 p, vec2 a, vec2 b) {
    vec2 ab = b - a;
    vec2 ap = p - a;
    float len2 = dot(ab, ab);
    if (len2 < EPSILON)
        return length(ap);
    float t = clamp(dot(ap, ab) / len2, 0.0, 1.0);
    return length(p - (a + t * ab));
}

#endif
