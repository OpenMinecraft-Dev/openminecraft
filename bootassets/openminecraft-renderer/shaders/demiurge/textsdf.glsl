#ifdef VERTEX_SHADER
#include "basics/geometry.glsl"
#include "basics/texelbuf.glsl"
#include "basics/vertexgen.glsl"

#vertex

layout(location = 0) out vec4 textColor;
layout(location = 1) out vec2 textUv;
layout(location = 2) flat out int textGlyphId;
layout(location = 3) out float textFactor;
layout(location = 4) out vec2 textPixelUv;

uniform ScreenData
{
    float width;
    float height;
}
ubo;
uniform samplerBuffer GlyphData;

vec4 getBBox()
{
    return vec4(texelFetchF(GlyphData, 0 + inTextGlyphId), texelFetchF(GlyphData, 1 + inTextGlyphId),
                texelFetchF(GlyphData, 2 + inTextGlyphId), texelFetchF(GlyphData, 3 + inTextGlyphId));
}

void main()
{
    // Quad is expanded around the glyph bbox to leave room for the SDF edge;
    // 1.5x is enough coverage while reducing overdraw.
    vec2 inPosition = (vertexgen_quad_normal() - 0.5) * 1.5 + 0.5;
    vec2 glyphCenter = inTextPos.xy + inTextPos.zw * 0.5;
    vec2 glyphSize = abs(inTextPos.zw) * inTextScale;
    vec2 screenPos = glyphCenter + (inPosition - 0.5) * glyphSize;

    gl_Position = vec4(geom_toNdc(screenPos, ubo.width, ubo.height), inTextDepth, 1.0);
    textColor = inTextColor;
    vec4 bb = getBBox();
    textUv = bb.xz + vec2(inPosition.x, 1 - inPosition.y) * (bb.yw - bb.xz);
    textGlyphId = inTextGlyphId;
    textFactor = inTextFactor;

    // UV units per screen pixel, used for subpixel (RGB) antialiasing offsets.
    textPixelUv = (bb.yw - bb.xz) / max(glyphSize, vec2(1.0));
}
#endif

#ifdef FRAGMENT_SHADER
#include "basics/texelbuf.glsl"

layout(location = 0) in vec4 textColor;
layout(location = 1) in vec2 textUv;
layout(location = 2) flat in int textGlyphId;
layout(location = 3) in float textFactor;
layout(location = 4) in vec2 textPixelUv;

layout(location = 0) out vec4 outColor;

uniform ScreenData
{
    float width;
    float height;
}
ubo;
uniform samplerBuffer GlyphData;

// ---- Glyph data accessors ----
// (offsets adjusted for leading bbox: [0..3] = bbox)
vec4 getBBox()
{
    return vec4(texelFetchF(GlyphData, 0 + textGlyphId), texelFetchF(GlyphData, 1 + textGlyphId),
                texelFetchF(GlyphData, 2 + textGlyphId), texelFetchF(GlyphData, 3 + textGlyphId));
}

int getOutlineCount()
{
    return int(texelFetchF(GlyphData, 4 + textGlyphId));
}

int getCurveCount(int outlineIndex)
{
    return int(texelFetchF(GlyphData, outlineIndex + 5 + textGlyphId));
}

vec2 getStartPoint(int outlineCount, int outlineIndex)
{
    int base = outlineCount + 5 + textGlyphId;
    return vec2(texelFetchF(GlyphData, base + outlineIndex * 2), texelFetchF(GlyphData, base + outlineIndex * 2 + 1));
}

vec4 getCurve(int outlineCount, int curveGlobalIndex)
{
    // data layout: [bbox(4)] [outlineCount(1)] [curveCounts(outlineCount)]
    //               [startPoints(2*outlineCount)] [curves(4*globalCurveCount)]
    // Total header size = 4 + 1 + outlineCount + 2*outlineCount = 5 + 3*outlineCount
    int headerFloats = 5 + 3 * outlineCount;
    int o = headerFloats + curveGlobalIndex * 4 + textGlyphId;
    return vec4(texelFetchF(GlyphData, o), texelFetchF(GlyphData, o + 1), texelFetchF(GlyphData, o + 2),
                texelFetchF(GlyphData, o + 3));
}

#include "basics/sdf/sdf_text.glsl"

float glyphDistance(vec2 uv)
{
    int outlineCount = getOutlineCount();
    int curveId = 0;
    float minDist = 1e10;
    int winding = 0;

    for (int i = 0; i < outlineCount; ++i)
    {
        int numCurves = getCurveCount(i);
        vec2 currentPos = getStartPoint(outlineCount, i);

        for (int j = 0; j < numCurves; ++j)
        {
            vec4 curve = getCurve(outlineCount, curveId);
            vec2 target = curve.xy;
            vec2 control = curve.zw;
            bool isLine = isinf(control.x);

            if (isLine)
            {
                minDist = min(minDist, sdf_distanceToLineSegment(uv, currentPos, target));
            }
            else
            {
                minDist = min(minDist, sdf_distanceToQuadraticBezier(uv, currentPos, control, target));
            }

            if (isLine)
            {
                float t;
                if (sdf_intersectLineY(uv.y, currentPos, target, t) > 0)
                {
                    float x = mix(currentPos.x, target.x, t);
                    if (x < uv.x)
                    {
                        float dydt = target.y - currentPos.y;
                        if (abs(dydt) > DERIVATIVE_THRESHOLD)
                        {
                            winding += (dydt > 0.0) ? 1 : -1;
                        }
                    }
                }
            }
            else
            {
                float t1, t2;
                int hits = sdf_intersectQuadraticY(uv.y, currentPos, control, target, t1, t2);
                for (int h = 0; h < hits; ++h)
                {
                    float t = (h == 0) ? t1 : t2;
                    float x = sdf_evalQuadraticX(t, currentPos, control, target);
                    if (x < uv.x)
                    {
                        float dydt = sdf_derivativeQuadraticY(t, currentPos, control, target);
                        if (abs(dydt) > DERIVATIVE_THRESHOLD)
                        {
                            winding += (dydt > 0.0) ? 1 : -1;
                        }
                    }
                }
            }

            currentPos = target;
            ++curveId;
        }
    }

    // Negative inside, positive outside.
    return minDist * (2.0 * step(0.5, abs(float(winding))) - 1.0);
}

void main()
{
    float dummy = ubo.width + ubo.height;
    vec4 bbox = getBBox();

    // Keep a margin around the glyph bbox so the anti-aliased fringe of outlines
    // that touch/overshoot the bbox is not clipped away.
    float bboxMargin = max(textFactor * 2.0, 1e-4);

    if (textUv.x < bbox.x - bboxMargin || textUv.x > bbox.y + bboxMargin || textUv.y < bbox.z - bboxMargin ||
        textUv.y > bbox.w + bboxMargin)
    {
        discard;
    }

    // Subpixel (RGB) antialiasing: evaluate the SDF at three horizontal sample
    // positions one third of a pixel apart, matching the physical RGB stripes.
    // This complements the natural grayscale coverage of the SDF.
    vec2 subpixel = vec2(textPixelUv.x / 3.0, 0.0);

    float distR = glyphDistance(textUv - subpixel);
    float distG = glyphDistance(textUv);
    float distB = glyphDistance(textUv + subpixel);

    vec3 coverage;
    coverage.r = smoothstep(-textFactor, textFactor, distR);
    coverage.g = smoothstep(-textFactor, textFactor, distG);
    coverage.b = smoothstep(-textFactor, textFactor, distB);

    // Blend the subpixel coverage with the plain grayscale coverage: subpixel
    // sharpens horizontal stems, the grayscale part keeps colored fringes mild.
    float gray = (coverage.r + coverage.g + coverage.b) / 3.0;
    const float subpixelStrength = 0.6;
    coverage = mix(vec3(gray), coverage, subpixelStrength);

    // Premultiplied per-channel coverage; the text pipeline blends with
    // (One, OneMinusSrcAlpha) so each color channel is attenuated independently.
    float alpha = max(max(coverage.r, coverage.g), coverage.b);
    outColor = vec4(textColor.rgb * coverage, textColor.a * alpha);
}
#endif
