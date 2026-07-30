#version 450 core
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

layout(location = 0) in vec2 outTexCoord;
layout(location = 0) out vec4 outColor;

layout(binding = 1) uniform sampler2D inTexture;
layout(std430, binding = 2) readonly buffer GlyphData {
    float d[];
} inGlyph;

int glyphFetchCount() {
    return int(inGlyph.d[0]);
}

int glyphFetchCurves(int id) {
    return int(inGlyph.d[id + 1]);
}

vec2 glyphFetchStartPoint(int count, int id) {
    return vec2(inGlyph.d[count + 1 + id * 2], inGlyph.d[count + 1 + id * 2 + 1]);
}

vec4 glyphFetchCurve(int count, int id) {
    return vec4(inGlyph.d[count * 3 + 1 + id * 4], inGlyph.d[count * 3 + 1 + id * 4 + 1], inGlyph.d[count * 3 + 1 + id * 4 + 2], inGlyph.d[count * 3 + 1 + id * 4 + 3]);
}

float distanceToQuadraticBezier(vec2 p, vec2 p0, vec2 p1, vec2 p2) {
    vec2 a = p1 - p0;
    vec2 b = p2 - 2.0 * p1 + p0;
    vec2 c = p0 - p;

    float t = clamp(dot(p - p0, p2 - p0) / max(dot(p2 - p0, p2 - p0), 1e-6), 0.0, 1.0);

    for (int i = 0; i < 4; i++) {
        vec2 pt = (1.0 - t) * (1.0 - t) * p0 + 2.0 * (1.0 - t) * t * p1 + t * t * p2;
        vec2 derivative = 2.0 * (1.0 - t) * (p1 - p0) + 2.0 * t * (p2 - p1);
        float f = dot(pt - p, derivative);
        float fPrime = dot(derivative, derivative) + dot(pt - p, 2.0 * (p2 - 2.0 * p1 + p0));
        if (abs(fPrime) < 1e-6) break;
        t -= f / fPrime;
        t = clamp(t, 0.0, 1.0);
    }

    vec2 closest = (1.0 - t) * (1.0 - t) * p0 + 2.0 * (1.0 - t) * t * p1 + t * t * p2;
    return length(p - closest);
}

float distanceToLineSegment(vec2 p, vec2 a, vec2 b) {
    vec2 ab = b - a;
    vec2 ap = p - a;
    float ab2 = dot(ab, ab);
    if (ab2 < 1e-6) {
        return length(ap);
    }
    float t = dot(ap, ab) / ab2;
    t = clamp(t, 0.0, 1.0);
    vec2 closest = a + t * ab;
    return length(p - closest);
}

#define INFINITY (1.0 / 0.0)

void main() {
    // outColor = texture(inTexture, outTexCoord);
    outColor = vec4(1.0);

    int curves = glyphFetchCount();
    int curveid = 0;
    vec2 currentPos = vec2(0.0);
    float dis = 100.0f;
    for (int i = 0; i < curves; ++i) {
        int numCurves = glyphFetchCurves(i);
        currentPos = glyphFetchStartPoint(curves, i);
        for (int j = 0; j < numCurves; ++j) {
            vec4 cvState = glyphFetchCurve(curves, curveid);
            vec2 target = cvState.xy;
            vec2 control = cvState.zw;
            if (control.x == INFINITY) {
                dis = min(dis, distanceToLineSegment(outTexCoord, currentPos, target));
            }
            else {
                dis = min(dis, distanceToQuadraticBezier(outTexCoord, currentPos, control, target));
            }
            currentPos = target;
            ++curveid;
        }
    }

    float viewDist = clamp(dis * 5.0, 0.0, 1.0);
    outColor = mix(vec4(0,0,0,1), vec4(1,1,1,1), viewDist);
}
