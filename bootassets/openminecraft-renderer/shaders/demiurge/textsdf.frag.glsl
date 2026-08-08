#version 330 core
#extension GL_ARB_separate_shader_objects : enable

#include "basics/texelbuf.glsl"

layout(location = 0) in vec4 textColor;
layout(location = 1) in vec2 textUv;
layout(location = 2) flat in int textGlyphId;
layout(location = 3) in float textFactor;

layout(location = 0) out vec4 outColor;

uniform ScreenData {
    float width;
    float height;
} ubo;
uniform samplerBuffer GlyphData;

// ---- Glyph data accessors ----
// (offsets adjusted for leading bbox: [0..3] = bbox)
vec4 getBBox() {
    return vec4(texelFetchF(GlyphData, 0 + textGlyphId), texelFetchF(GlyphData, 1 + textGlyphId), texelFetchF(GlyphData, 2 + textGlyphId), texelFetchF(GlyphData, 3 + textGlyphId));
}

int getOutlineCount() {
    return int(texelFetchF(GlyphData, 4 + textGlyphId));
}

int getCurveCount(int outlineIndex) {
    return int(texelFetchF(GlyphData, outlineIndex + 5 + textGlyphId));
}

vec2 getStartPoint(int outlineCount, int outlineIndex) {
    int base = outlineCount + 5 + textGlyphId;
    return vec2(texelFetchF(GlyphData, base + outlineIndex * 2),
                texelFetchF(GlyphData, base + outlineIndex * 2 + 1));
}

vec4 getCurve(int outlineCount, int curveGlobalIndex) {
    // data layout: [bbox(4)] [outlineCount(1)] [curveCounts(outlineCount)]
    //               [startPoints(2*outlineCount)] [curves(4*globalCurveCount)]
    // Total header size = 4 + 1 + outlineCount + 2*outlineCount = 5 + 3*outlineCount
    int headerFloats = 5 + 3 * outlineCount;
    int o = headerFloats + curveGlobalIndex * 4 + textGlyphId;
    return vec4(texelFetchF(GlyphData, o), texelFetchF(GlyphData, o + 1), texelFetchF(GlyphData, o + 2), texelFetchF(GlyphData, o + 3));
}

#include "basics/sdf/sdf_text.glsl"

void main() {
    float dummy = ubo.width + ubo.height;
    vec4 bbox = getBBox();
    if (textUv.x < bbox.x || textUv.x > bbox.y || textUv.y < bbox.z || textUv.y > bbox.w) {
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
                minDist = min(minDist, sdf_distanceToLineSegment(textUv, currentPos, target));
            } else {
                minDist = min(minDist, sdf_distanceToQuadraticBezier(textUv, currentPos, control, target));
            }

            // Winding number contribution
            if (isLine) {
                float t;
                if (sdf_intersectLineY(textUv.y, currentPos, target, t) > 0) {
                    float x = mix(currentPos.x, target.x, t);
                    if (x < textUv.x) {
                        float dydt = target.y - currentPos.y;
                        if (abs(dydt) > DERIVATIVE_THRESHOLD) {
                            winding += (dydt > 0.0) ? 1 : -1;
                        }
                    }
                }
            } else {
                float t1, t2;
                int hits = sdf_intersectQuadraticY(textUv.y, currentPos, control, target, t1, t2);
                for (int h = 0; h < hits; ++h) {
                    float t = (h == 0) ? t1 : t2;
                    float x = sdf_evalQuadraticX(t, currentPos, control, target);
                    if (x < textUv.x) {
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

    outColor = textColor * smoothstep(-textFactor, 0.0, minDist);
}
