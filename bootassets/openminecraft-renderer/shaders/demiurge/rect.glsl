#ifdef VERTEX_SHADER
#include "basics/geometry.glsl"
#include "basics/vertexgen.glsl"

#vertex

layout(location = 0) out vec4 rectColor;

uniform ScreenData
{
    float width;
    float height;
}
ubo;

void main()
{
    vec2 inPosition = vertexgen_quad_normal();
    vec2 center = geom_rectCenterPos(inRectPos);
    vec2 halfSize = abs(inRectPos.zw) * 0.5 * inRectScale;
    vec2 screenPos = center - halfSize + inPosition.xy * (halfSize * 2.0);

    gl_Position = vec4(geom_toNdc(screenPos, ubo.width, ubo.height), inRectDepth, 1.0);
    rectColor = inRectColor;
}
#endif

#ifdef FRAGMENT_SHADER
layout(location = 0) in vec4 rectColor;
layout(location = 0) out vec4 outColor;

void main()
{
    outColor = rectColor;
}
#endif