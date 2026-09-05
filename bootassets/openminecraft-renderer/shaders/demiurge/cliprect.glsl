#ifdef VERTEX_SHADER
#include "basics/geometry.glsl"
#include "basics/vertexgen.glsl"

#vertex

layout(location = 0) out vec2 targetScreenPos;
layout(location = 1) out vec4 rectPos;

uniform ScreenData
{
    float width;
    float height;
}
ubo;

void main()
{
    vec2 inPosition = vertexgen_quad_normal();
    vec2 screenPos = inPosition.xy * vec2(ubo.width, ubo.height);

    gl_Position = vec4(geom_toNdc(screenPos, ubo.width, ubo.height), inRectDepth - 0.1, 1.0);
    rectPos = inRectPos;
    targetScreenPos = screenPos;
}
#endif

#ifdef FRAGMENT_SHADER
layout(location = 0) in vec2 targetScreenPos;
layout(location = 1) in vec4 rectPos;
layout(location = 0) out vec4 outColor;

void main()
{
    if (!(targetScreenPos.x < rectPos.x || targetScreenPos.x > rectPos.x + rectPos.z || targetScreenPos.y < rectPos.y ||
          targetScreenPos.y > rectPos.y + rectPos.w))
    {
        discard;
    }
}
#endif