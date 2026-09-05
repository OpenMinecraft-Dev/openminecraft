#include "basics/texelbuf.glsl"

uniform samplerBuffer inSvgData;

#ifdef VERTEX_SHADER
#include "basics/vertexgen.glsl"

layout(location = 0) out vec2 svgGlyphPos;

void main()
{
    svgGlyphPos = vertexgen_quad_normal() * vec2(1.0, -1.0) + vec2(0.0, 1.0);
    gl_Position = vec4(vertexgen_quad_normal() * vec2(0.25), 1.0, 1.0);
}
#endif

#ifdef FRAGMENT_SHADER
#include "basics/sdf/sdf_text.glsl"
float curType;
float curParams[6];

layout(location = 0) in vec2 svgGlyphPos;
layout(location = 0) out vec4 outColor;

void main()
{
    int idx = 0;
    int outlineCount = int(texelFetchF(inSvgData, (idx)));
    idx++;

    float minDist = 1.0;
    int winding = 0;
    for (int i = 0; i < outlineCount; i++)
    {
        vec2 start = vec2(texelFetchF(inSvgData, (idx)), texelFetchF(inSvgData, (idx + 1)));
        minDist = min(minDist, distance(svgGlyphPos, start));
        idx += 2;

        int curveCount = int(texelFetchF(inSvgData, (idx)));
        idx++;

        vec2 pointer = start;
        for (int j = 0; j < curveCount; j++)
        {
            curType = texelFetchF(inSvgData, (idx));
            idx++;

            if (curType == 0.0)
            {
                vec2 target = vec2(texelFetchF(inSvgData, (idx)), texelFetchF(inSvgData, (idx + 1)));
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
                            winding += (dydt > 0.0) ? 1 : -1;
                        }
                    }
                }
                pointer = target;

                idx += 2;
            }
            else if (curType == 1.0)
            {
                vec2 target = vec2(texelFetchF(inSvgData, (idx)), texelFetchF(inSvgData, (idx + 1)));
                vec2 control = vec2(texelFetchF(inSvgData, (idx + 2)), texelFetchF(inSvgData, (idx + 3)));
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
                            winding += (dydt > 0.0) ? 1 : -1;
                        }
                    }
                }
                pointer = target;
                idx += 4;
            }
            else if (curType == 2.0)
            {
                vec2 target = vec2(texelFetchF(inSvgData, (idx)), texelFetchF(inSvgData, (idx + 1)));
                vec2 control1 = vec2(texelFetchF(inSvgData, (idx + 2)), texelFetchF(inSvgData, (idx + 3)));
                vec2 control2 = vec2(texelFetchF(inSvgData, (idx + 4)), texelFetchF(inSvgData, (idx + 5)));
                minDist = min(minDist, sdf_distanceToCubicBezier(svgGlyphPos, pointer, control1, control2, target));
                winding = sdf_windingCubic(svgGlyphPos, pointer, control1, control2, target, winding);
                pointer = target;
                idx += 6;
            }
            else if (curType == 3.0)
            {
                vec2 target = vec2(texelFetchF(inSvgData, (idx)), texelFetchF(inSvgData, (idx + 1)));
                float rx = texelFetchF(inSvgData, (idx + 2));
                float ry = texelFetchF(inSvgData, (idx + 3));
                float xrot = texelFetchF(inSvgData, (idx + 4));
                int flgs = int(texelFetchF(inSvgData, (idx + 5)));
                bool largeArcFlag = bool((flgs >> 1) & 1);
                bool sweepFlag = bool(flgs & 1);
                minDist = min(minDist,
                              sdf_distanceToArc(svgGlyphPos, pointer, target, rx, ry, xrot, largeArcFlag, sweepFlag));
                winding = sdf_windingArc(svgGlyphPos, pointer, target, rx, ry, xrot, largeArcFlag, sweepFlag, winding);
                pointer = target;
                idx += 6;
            }
        }
    }

    minDist = minDist * (2.0 * step(0.5, abs(float(winding))) - 1.0);

    outColor = vec4(vec3(1.0), smoothstep(-0.005, 0.005, minDist));
}
#endif