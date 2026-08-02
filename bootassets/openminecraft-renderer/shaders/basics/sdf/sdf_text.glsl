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

float sdf_distanceToQuadraticBezier(vec2 p, vec2 p0, vec2 p1, vec2 p2) {
    vec2 a = p1 - p0;
    vec2 b = p2 - 2.0 * p1 + p0;
    vec2 c = p0 - p;

    float t = clamp(dot(p - p0, p2 - p0) / max(dot(p2 - p0, p2 - p0), EPSILON), 0.0, 1.0);

    for (int i = 0; i < ITERATION; ++i) {
        vec2 pt = mix(mix(p0, p1, t), mix(p1, p2, t), t);
        vec2 derivative = 2.0 * mix(p1 - p0, p2 - p1, t);
        float f = dot(pt - p, derivative);
        float fPrime = dot(derivative, derivative) + dot(pt - p, 2.0 * (p2 - 2.0 * p1 + p0));
        if (abs(fPrime) < EPSILON)
            break;
        t -= f / fPrime;
        t = clamp(t, 0.0, 1.0);
    }

    vec2 closest = mix(mix(p0, p1, t), mix(p1, p2, t), t);
    return length(p - closest);
}

int sdf_intersectLineY(float rayY, vec2 a, vec2 b, out float t) {
    t = -1.0;
    float dy = b.y - a.y;
    if (abs(dy) < EPSILON)
        return 0;
    t = (rayY - a.y) / dy;
    return (t > 0.0 && t <= 1.0) ? 1 : 0;
}

int sdf_intersectQuadraticY(float rayY, vec2 p0, vec2 p1, vec2 p2, out float t1, out float t2) {
    float a = p0.y - 2.0 * p1.y + p2.y;
    float b = 2.0 * (p1.y - p0.y);
    float c = p0.y - rayY;
    t1 = t2 = -1.0;

    if (abs(a) < EPSILON) {
        if (abs(b) < EPSILON) return 0;
        float t = -c / b;
        if (t >= 0.0 && t <= 1.0) { t1 = t; return 1; }
        return 0;
    }

    float disc = b * b - 4.0 * a * c;
    if (disc < 0.0) return 0;

    float sqrtD = sqrt(disc);
    float inv2a = 0.5 / a;
    float r1 = (-b - sqrtD) * inv2a;
    float r2 = (-b + sqrtD) * inv2a;

    if (r1 > r2) { float tmp = r1; r1 = r2; r2 = tmp; }

    int count = 0;
    if (r1 >= 0.0 && r1 <= 1.0) { t1 = r1; count = 1; }
    if (r2 >= 0.0 && r2 <= 1.0 && (count == 0 || r2 - r1 > EPSILON)) {
        if (count == 0) t1 = r2;
        else t2 = r2;
        count++;
    }
    return count;
}

float sdf_evalQuadraticX(float t, vec2 p0, vec2 p1, vec2 p2) {
    float mt = 1.0 - t;
    return mt * mt * p0.x + 2.0 * mt * t * p1.x + t * t * p2.x;
}

float sdf_derivativeQuadraticY(float t, vec2 p0, vec2 p1, vec2 p2) {
    return 2.0 * mix(p1.y - p0.y, p2.y - p1.y, t);
}

#endif
