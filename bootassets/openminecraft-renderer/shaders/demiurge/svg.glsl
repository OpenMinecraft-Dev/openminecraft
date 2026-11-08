#include "basics/texelbuf.glsl"

uniform ScreenData
{
    float width;
    float height;
}
ubo;
uniform samplerBuffer SvgData;

#ifdef VERTEX_SHADER
#include "basics/vertexgen.glsl"
#include "basics/geometry.glsl"

#vertex

layout(location = 0) out vec2 svgGlyphPos;
layout(location = 1) flat out int svgIndex;
layout(location = 2) out vec4 svgColor;
layout(location = 3) flat out float svgFactor;

void main()
{
    vec2 inPosition = vertexgen_quad_normal();
    vec2 screenPos = inSvgPos.xy - vec2(10) + inPosition.xy * (inSvgPos.zw + vec2(20));

    // Glyph coordinates are normalized to 0..1 inside the node rect (compile() divides
    // by svgSize), so map the expanded quad back into rect-local space here.
    svgGlyphPos = (screenPos - inSvgPos.xy) / inSvgPos.zw;
    svgIndex = inSvgGlyphId;
    svgColor = inSvgColor;
    svgFactor = inSvgFactor;
    gl_Position = vec4(geom_toNdc(screenPos, ubo.width, ubo.height), inSvgDepth, 1.0);
}
#endif

#ifdef FRAGMENT_SHADER
#include "basics/sdf/sdf_text.glsl"

layout(location = 0) in vec2 svgGlyphPos;
layout(location = 1) flat in int svgIndex;
layout(location = 2) in vec4 svgColor;
layout(location = 3) flat in float svgFactor;

layout(location = 0) out vec4 outColor;

void main()
{
    float unused = ubo.width;

    int idx = svgIndex;
    int outlineCount = int(texelFetchF(SvgData, (idx)));
    idx++;

    float minDist = 1e308;
    int winding = 0;
    bool evenodd = false;
    for (int i = 0; i < outlineCount; i++)
    {
        vec2 start = vec2(texelFetchF(SvgData, (idx)), texelFetchF(SvgData, (idx + 1)));
        minDist = min(minDist, distance(svgGlyphPos, start));
        idx += 2;

        int curveCount = int(texelFetchF(SvgData, (idx)));
        idx++;

        vec2 pointer = start;
        for (int j = 0; j < curveCount; j++)
        {
            float curType = texelFetchF(SvgData, (idx));
            idx++;

            if (curType == 0.0)
            {
                vec2 target = vec2(texelFetchF(SvgData, (idx)), texelFetchF(SvgData, (idx + 1)));
                minDist = min(minDist, sdf_distanceToLineSegment(svgGlyphPos, pointer, target));

                float t;
                if (sdf_intersectLineY(svgGlyphPos.y, pointer, target, t) > 0)
                {
                    float x = mix(pointer.x, target.x, t);
                    if (x < svgGlyphPos.x)
                    {
                        float dydt = target.y - pointer.y;
                        if (abs(dydt) > 1e-5)
                        {
                            winding += (dydt > 0.0 || evenodd) ? 1 : -1;
                        }
                    }
                }
                pointer = target;

                idx += 2;
            }
            else if (curType == 1.0)
            {
                vec2 target = vec2(texelFetchF(SvgData, (idx)), texelFetchF(SvgData, (idx + 1)));
                vec2 control = vec2(texelFetchF(SvgData, (idx + 2)), texelFetchF(SvgData, (idx + 3)));
                minDist = min(minDist, sdf_distanceToQuadraticBezier(svgGlyphPos, pointer, control, target));
                float t1, t2;
                int hits = sdf_intersectQuadraticY(svgGlyphPos.y, pointer, control, target, t1, t2);
                for (int h = 0; h < hits; ++h)
                {
                    float t = (h == 0) ? t1 : t2;
                    float x = sdf_evalQuadraticX(t, pointer, control, target);
                    if (x < svgGlyphPos.x)
                    {
                        float dydt = sdf_derivativeQuadraticY(t, pointer, control, target);
                        if (abs(dydt) > 1e-5)
                        {
                            winding += (dydt > 0.0 || evenodd) ? 1 : -1;
                        }
                    }
                }
                pointer = target;
                idx += 4;
            }
            else if (curType == 2.0)
            {
                vec2 target = vec2(texelFetchF(SvgData, (idx)), texelFetchF(SvgData, (idx + 1)));
                vec2 control1 = vec2(texelFetchF(SvgData, (idx + 2)), texelFetchF(SvgData, (idx + 3)));
                vec2 control2 = vec2(texelFetchF(SvgData, (idx + 4)), texelFetchF(SvgData, (idx + 5)));
                minDist = min(minDist, sdf_distanceToCubicBezier(svgGlyphPos, pointer, control1, control2, target));
                winding = sdf_windingCubic(svgGlyphPos, pointer, control1, control2, target, winding, evenodd);
                pointer = target;
                idx += 6;
            }
            else if (curType == 3.0)
            {
                vec2 target = vec2(texelFetchF(SvgData, (idx)), texelFetchF(SvgData, (idx + 1)));
                float rx = texelFetchF(SvgData, (idx + 2));
                float ry = texelFetchF(SvgData, (idx + 3));
                float xrot = texelFetchF(SvgData, (idx + 4));
                int flgs = int(texelFetchF(SvgData, (idx + 5)));
                bool largeArcFlag = bool((flgs >> 1) & 1);
                bool sweepFlag = bool(flgs & 1);
                minDist = min(minDist,
                              sdf_distanceToArc(svgGlyphPos, pointer, target, rx, ry, xrot, largeArcFlag, sweepFlag));
                winding = sdf_windingArc(svgGlyphPos, pointer, target, rx, ry, xrot, largeArcFlag, sweepFlag, winding,
                                         evenodd);
                pointer = target;
                idx += 6;
            }
        }
    }

    if (!evenodd)
    {
        minDist = minDist * (2.0 * step(0.5, abs(float(winding))) - 1.0);
    }
    else
    {
        minDist = minDist * ((float(winding % 2) - 0.5) * 2);
    }

    outColor = svgColor * smoothstep(-svgFactor, svgFactor, minDist);
}
#endif