#version 450 core
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in vec4 inColor;
layout(location = 1) in vec2 texCoord;
layout(location = 2) flat in int inTextGlyphId;
layout(location = 3) in float inTextFactor;

layout(location = 0) out vec4 outColor;

layout(std430, binding = 1) readonly buffer GlyphData {
    float data[];
} glyphBuffer;

// ---- Glyph data accessors ----
// (offsets adjusted for leading bbox: [0..3] = bbox)
vec4 getBBox() {
    return vec4(glyphBuffer.data[0 + inTextGlyphId], glyphBuffer.data[1 + inTextGlyphId],
                glyphBuffer.data[2 + inTextGlyphId], glyphBuffer.data[3 + inTextGlyphId]);
}

int getOutlineCount() {
    return int(glyphBuffer.data[4 + inTextGlyphId]);
}

int getCurveCount(int outlineIndex) {
    return int(glyphBuffer.data[outlineIndex + 5 + inTextGlyphId]);
}

vec2 getStartPoint(int outlineCount, int outlineIndex) {
    int base = outlineCount + 5 + inTextGlyphId;
    return vec2(glyphBuffer.data[base + outlineIndex * 2],
                glyphBuffer.data[base + outlineIndex * 2 + 1]);
}

vec4 getCurve(int outlineCount, int curveGlobalIndex) {
    // data layout: [bbox(4)] [outlineCount(1)] [curveCounts(outlineCount)]
    //               [startPoints(2*outlineCount)] [curves(4*globalCurveCount)]
    // Total header size = 4 + 1 + outlineCount + 2*outlineCount = 5 + 3*outlineCount
    int headerFloats = 5 + 3 * outlineCount;
    int offset = headerFloats + curveGlobalIndex * 4 + inTextGlyphId;
    return vec4(glyphBuffer.data[offset], glyphBuffer.data[offset + 1],
                glyphBuffer.data[offset + 2], glyphBuffer.data[offset + 3]);
}

#include "basics/sdf/sdf_text.glsl"

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
                minDist = min(minDist, sdf_distanceToLineSegment(texCoord, currentPos, target));
            } else {
                minDist = min(minDist, sdf_distanceToQuadraticBezier(texCoord, currentPos, control, target));
            }

            // Winding number contribution
            if (isLine) {
                float t;
                if (sdf_intersectLineY(texCoord.y, currentPos, target, t) > 0) {
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
                int hits = sdf_intersectQuadraticY(texCoord.y, currentPos, control, target, t1, t2);
                for (int h = 0; h < hits; ++h) {
                    float t = (h == 0) ? t1 : t2;
                    float x = sdf_evalQuadraticX(t, currentPos, control, target);
                    if (x < texCoord.x) {
                        float dydt = sdf_derivativeQuadraticY(t, currentPos, control, target);
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

    outColor = inColor * smoothstep(-inTextFactor, 0.0, minDist);
}
