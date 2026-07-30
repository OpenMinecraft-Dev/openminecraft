#version 450 core
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

precision highp float;

layout(location = 0) in vec2 texCoord;
layout(location = 0) out vec4 fragColor;

layout(binding = 1) uniform sampler2D inTexture;
layout(std430, binding = 2) readonly buffer GlyphData {
    float data[];
} glyphBuffer;

// ---- Constants ----
const float INF = 1.0 / 0.0;
const float EPSILON = 1e-6;
const float DERIVATIVE_THRESHOLD = 1e-12;
const float SMOOTH_WIDTH = 0.01;
const int ITERATION = 3;

// ---- Glyph data accessors ----
// (offsets adjusted for leading bbox: [0..3] = bbox)
vec4 getBBox() {
    return vec4(glyphBuffer.data[0], glyphBuffer.data[1],
                glyphBuffer.data[2], glyphBuffer.data[3]);
}

int getOutlineCount() {
    return int(glyphBuffer.data[4]);
}

int getCurveCount(int outlineIndex) {
    return int(glyphBuffer.data[outlineIndex + 5]);
}

vec2 getStartPoint(int outlineCount, int outlineIndex) {
    int base = outlineCount + 5;
    return vec2(glyphBuffer.data[base + outlineIndex * 2],
                glyphBuffer.data[base + outlineIndex * 2 + 1]);
}

vec4 getCurve(int outlineCount, int curveGlobalIndex) {
    // data layout: [bbox(4)] [outlineCount(1)] [curveCounts(outlineCount)]
    //               [startPoints(2*outlineCount)] [curves(4*globalCurveCount)]
    // Total header size = 4 + 1 + outlineCount + 2*outlineCount = 5 + 3*outlineCount
    int headerFloats = 5 + 3 * outlineCount;
    int offset = headerFloats + curveGlobalIndex * 4;
    return vec4(glyphBuffer.data[offset], glyphBuffer.data[offset + 1],
                glyphBuffer.data[offset + 2], glyphBuffer.data[offset + 3]);
}

// ---- Geometry helpers ----
float distanceToLineSegment(vec2 p, vec2 a, vec2 b) {
    vec2 ab = b - a;
    vec2 ap = p - a;
    float len2 = dot(ab, ab);
    if (len2 < EPSILON)
        return length(ap);
    float t = clamp(dot(ap, ab) / len2, 0.0, 1.0);
    return length(p - (a + t * ab));
}

float distanceToQuadraticBezier(vec2 p, vec2 p0, vec2 p1, vec2 p2) {
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

// ---- Intersection tests for winding ----
int intersectLineY(float rayY, vec2 a, vec2 b, out float t) {
    t = -1.0;
    float dy = b.y - a.y;
    if (abs(dy) < EPSILON)
        return 0;
    t = (rayY - a.y) / dy;
    return (t > 0.0 && t <= 1.0) ? 1 : 0;
}

int intersectQuadraticY(float rayY, vec2 p0, vec2 p1, vec2 p2, out float t1, out float t2) {
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

float evalQuadraticX(float t, vec2 p0, vec2 p1, vec2 p2) {
    float mt = 1.0 - t;
    return mt * mt * p0.x + 2.0 * mt * t * p1.x + t * t * p2.x;
}

float derivativeQuadraticY(float t, vec2 p0, vec2 p1, vec2 p2) {
    return 2.0 * mix(p1.y - p0.y, p2.y - p1.y, t);
}

// ---- Main ----
void main() {
    vec4 bbox = getBBox();
    if (texCoord.x < bbox.x || texCoord.x > bbox.y || texCoord.y < bbox.z || texCoord.y > bbox.w) {
        discard;
    }

    int outlineCount = getOutlineCount();
    int curveId = 0;
    float minDist = 1e10;
    int winding = 0;
    int intersects = 0;

    for (int i = 0; i < outlineCount; ++i) {
        int numCurves = getCurveCount(i);
        vec2 currentPos = getStartPoint(outlineCount, i);

        for (int j = 0; j < numCurves; ++j) {
            vec4 curve = getCurve(outlineCount, curveId);
            vec2 target = curve.xy;
            vec2 control = curve.zw;
            bool isLine = isinf(control.x);

            // Update minimum distance
            if (isLine) {
                minDist = min(minDist, distanceToLineSegment(texCoord, currentPos, target));
            } else {
                minDist = min(minDist, distanceToQuadraticBezier(texCoord, currentPos, control, target));
            }

            // Winding number contribution
            if (isLine) {
                float t;
                if (intersectLineY(texCoord.y, currentPos, target, t) > 0) {
                    float x = mix(currentPos.x, target.x, t);
                    if (x < texCoord.x) {
                        float dydt = target.y - currentPos.y;
                        if (abs(dydt) > DERIVATIVE_THRESHOLD) {
                            winding += (dydt > 0.0) ? 1 : -1;
                        }
                    }
                }
            } else {
                float t1, t2;
                int hits = intersectQuadraticY(texCoord.y, currentPos, control, target, t1, t2);
                for (int h = 0; h < hits; ++h) {
                    float t = (h == 0) ? t1 : t2;
                    float x = evalQuadraticX(t, currentPos, control, target);
                    if (x < texCoord.x) {
                        float dydt = derivativeQuadraticY(t, currentPos, control, target);
                        if (abs(dydt) > DERIVATIVE_THRESHOLD) {
                            winding += (dydt > 0.0) ? 1 : -1;
                        }
                    }
                }
            }

            currentPos = target;
            ++curveId;
        }
    }

    // Determine inside/outside
    minDist = minDist * (2.0 * step(0.5, abs(float(winding))) - 1.0);

    fragColor = vec4(1.0) * smoothstep(0.0, SMOOTH_WIDTH, minDist);
}
