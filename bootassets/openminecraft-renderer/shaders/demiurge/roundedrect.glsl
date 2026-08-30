#ifdef VERTEX_SHADER
#include "basics/geometry.glsl"
#include "basics/rotation.glsl"
#include "basics/vertexgen.glsl"

#vertex

layout(location = 0) out vec4 rrectColor;
layout(location = 1) out vec2 rrectPosition;
layout(location = 2) out vec4 rrectRadius;
layout(location = 3) out vec4 rrectRectPosition;
layout(location = 4) out float rrectFactor;

uniform ScreenData
{
    float width;
    float height;
}
ubo;

void main()
{
    vec2 inPosition = vertexgen_quad_normal();
    vec2 screenPos = inRectPos.xy - vec2(10) + inPosition.xy * (inRectPos.zw + vec2(20));
    vec2 rectCenter = geom_rectCenterPos(inRectPos);
    vec3 localPos = vec3(screenPos - rectCenter, 0.0);

    gl_Position = vec4(geom_toNdc(rectCenter + rotation_quat(inRectRotation, localPos).xy, ubo.width, ubo.height),
                       inRectDepth, 1.0);
    rrectColor = inRectColor;
    rrectPosition = screenPos;
    rrectRadius = inRectRadius;
    rrectRectPosition = inRectPos;
    rrectFactor = inRectFactor;
}
#endif

#ifdef FRAGMENT_SHADER
#include "basics/sdf/sdf_rrect.glsl"

layout(location = 0) in vec4 rrectColor;
layout(location = 1) in vec2 rrectPosition;
layout(location = 2) in vec4 rrectRadius;
layout(location = 3) in vec4 rrectRectPosition;
layout(location = 4) in float rrectFactor;

layout(location = 0) out vec4 outColor;

void main()
{
    vec2 rectCenter = geom_rectCenterPos(rrectRectPosition);
    vec2 halfSize = rrectRectPosition.zw / 2.0;
    vec2 localPos = rrectPosition - rectCenter;

    float alpha = sdf_rrect(localPos, halfSize, rrectRadius, 0.0, rrectFactor);

    outColor = rrectColor * alpha;
}
#endif