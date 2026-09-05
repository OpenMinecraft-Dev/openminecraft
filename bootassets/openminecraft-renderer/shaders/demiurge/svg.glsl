#include "basics/texelbuf.glsl"

uniform samplerBuffer inSvgData;

#ifdef VERTEX_SHADER
#include "basics/vertexgen.glsl"

layout(location = 0) out vec2 svgGlyphPos;

void main()
{
    svgGlyphPos = vertexgen_quad_normal();
    gl_Position = vec4(vertexgen_quad_normal(), 1.0, 1.0);
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

    float dd = 1.0;

    for (int i = 0; i < outlineCount; i++)
    {
        vec2 start = vec2(texelFetchF(inSvgData, (idx)), texelFetchF(inSvgData, (idx + 1)));
        dd = min(dd, distance(svgGlyphPos, start));
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
                dd = min(dd, sdf_distanceToLineSegment(svgGlyphPos, pointer, target));
                pointer = target;
                idx += 2;
            }
            else if (curType == 1.0)
            {
                vec2 target = vec2(texelFetchF(inSvgData, (idx)), texelFetchF(inSvgData, (idx + 1)));
                vec2 control1 = vec2(texelFetchF(inSvgData, (idx + 2)), texelFetchF(inSvgData, (idx + 3)));
                dd = min(dd, sdf_distanceToQuadraticBezier(svgGlyphPos, pointer, control1, target));
                pointer = target;
                idx += 4;
            }
            else if (curType == 2.0)
            {
                vec2 target = vec2(texelFetchF(inSvgData, (idx)), texelFetchF(inSvgData, (idx + 1)));
                vec2 control1 = vec2(texelFetchF(inSvgData, (idx + 2)), texelFetchF(inSvgData, (idx + 3)));
                vec2 control2 = vec2(texelFetchF(inSvgData, (idx + 4)), texelFetchF(inSvgData, (idx + 5)));
                dd = min(dd, sdf_distanceToCubicBezier(svgGlyphPos, pointer, control1, control2, target));
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
                dd = min(dd, sdf_distanceToArc(svgGlyphPos, pointer, target, rx, ry, xrot, largeArcFlag, sweepFlag));
                pointer = target;
                idx += 6;
            }
        }
    }

    outColor = vec4(vec3(dd), 1.0);
}
#endif