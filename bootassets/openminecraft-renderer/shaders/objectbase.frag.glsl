#version 450 core
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

precision highp float;

layout(location = 0) in vec2 outTexCoord;
layout(location = 0) out vec4 outColor;

layout(binding = 1) uniform sampler2D inTexture;
layout(std430, binding = 2) readonly buffer GlyphData {
    float d[];
} inGlyph;

vec4 glyphFetchBBox() {
    return vec4(inGlyph.d[0], inGlyph.d[1], inGlyph.d[2], inGlyph.d[3]);
}

int glyphFetchCount() {
    return int(inGlyph.d[0 + 4]);
}

int glyphFetchCurves(int id) {
    return int(inGlyph.d[id + 1 + 4]);
}

vec2 glyphFetchStartPoint(int count, int id) {
    return vec2(inGlyph.d[count + 1 + 4 + id * 2], inGlyph.d[count + 1 + 4 + id * 2 + 1]);
}

vec4 glyphFetchCurve(int count, int id) {
    return vec4(inGlyph.d[count * 3 + 1 + 4 + id * 4], inGlyph.d[count * 3 + 1 + 4 + id * 4 + 1], inGlyph.d[count * 3 + 1 + 4 + id * 4 + 2], inGlyph.d[count * 3 + 1 + 4 + id * 4 + 3]);
}

float distanceToQuadraticBezier(vec2 p, vec2 p0, vec2 p1, vec2 p2) {
    vec2 a = p1 - p0;
    vec2 b = p2 - 2.0 * p1 + p0;
    vec2 c = p0 - p;

    float t = clamp(dot(p - p0, p2 - p0) / max(dot(p2 - p0, p2 - p0), 1e-6), 0.0, 1.0);

    for (int i = 0; i < 3; i++) {
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

int intersectQuadraticY(float rayY, vec2 p0, vec2 p1, vec2 p2, out float t1, out float t2) {
    float a = p0.y - 2.0 * p1.y + p2.y;
    float b = 2.0 * (p1.y - p0.y);
    float c = p0.y - rayY;
    t1 = t2 = -1.0;

    if (abs(a) < 1e-6) {
        if (abs(b) < 1e-6) return 0;
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
    if (r2 >= 0.0 && r2 <= 1.0 && (count == 0 || r2 - r1 > 1e-6)) {
        if (count == 0) t1 = r2;
        else t2 = r2;
        count++;
    }
    return count;
}

float evalQuadraticX(float t, vec2 p0, vec2 p1, vec2 p2) {
    return (1.0 - t) * (1.0 - t) * p0.x + 2.0 * (1.0 - t) * t * p1.x + t * t * p2.x;
}

float derivativeQuadraticY(float t, vec2 p0, vec2 p1, vec2 p2) {
    return 2.0 * (1.0 - t) * (p1.y - p0.y) + 2.0 * t * (p2.y - p1.y);
}

int intersectLineY(float rayY, vec2 a, vec2 b, out float t) {
    t = -1.0;
    if (abs(b.y - a.y) < 1e-6) {
        return 0;
    }
    t = (rayY - a.y) / (b.y - a.y);
    if (t > 0.0 && t <= 1.0) return 1;
    return 0;
}

void main() {
    // outColor = texture(inTexture, outTexCoord);
    outColor = vec4(1.0);

    vec4 bbox = glyphFetchBBox();
    if (outTexCoord.x < bbox.x || outTexCoord.x > bbox.y || outTexCoord.y < bbox.z || outTexCoord.y > bbox.w) {
        discard;
    }

    int curves = glyphFetchCount();
    int curveid = 0;
    vec2 currentPos = vec2(0.0);
    float dis = 100.0f;
    int winding = 0;
    for (int i = 0; i < curves; ++i) {
        int numCurves = glyphFetchCurves(i);
        currentPos = glyphFetchStartPoint(curves, i);
        for (int j = 0; j < numCurves; ++j) {
            vec4 cvState = glyphFetchCurve(curves, curveid);
            vec2 target = cvState.xy;
            vec2 control = cvState.zw;

            bool isLine = isinf(control.x);

            if (isLine) {
                dis = min(dis, distanceToLineSegment(outTexCoord, currentPos, target));
            }
            else {
                dis = min(dis, distanceToQuadraticBezier(outTexCoord, currentPos, control, target));
            }

            if (isLine) {
                float tLine;
                if (intersectLineY(outTexCoord.y, currentPos, target, tLine) > 0) {
                    float x = currentPos.x + tLine * (target.x - currentPos.x);
                    if (x < outTexCoord.x) {
                        float dydt = target.y - currentPos.y;
                        winding += (dydt > 0.0) ? 1 : -1;
                    }
                }
            } else {
                float t1, t2;
                int hits = intersectQuadraticY(outTexCoord.y, currentPos, control, target, t1, t2);
                for (int h = 0; h < hits; h++) {
                    float t = (h == 0) ? t1 : t2;
                    float x = evalQuadraticX(t, currentPos, control, target);
                    if (x < outTexCoord.x) {
                        float dydt = derivativeQuadraticY(t, currentPos, control, target);
                        if (abs(dydt) > 1e-12) {
                            winding += (dydt > 0.0) ? 1 : -1;
                        }
                    }
                }
            }

            currentPos = target;
            ++curveid;
        }
    }

    if (winding == 0) {
        dis = -dis;
    }

    outColor *= (smoothstep(0.0, 0.01, dis));
}
