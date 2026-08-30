#ifdef VERTEX_SHADER
#include "basics/geometry.glsl"
#include "basics/rotation.glsl"
#include "basics/vertexgen.glsl"

#vertex

layout(location = 0) out vec4 secColor;
layout(location = 1) out vec2 secPosition;
layout(location = 2) out vec4 secSectorPosition;
layout(location = 3) out vec2 secSectorAngle;
layout(location = 4) out float secSectorFactor;

uniform ScreenData
{
    float width;
    float height;
}
ubo;

void main()
{
    vec2 inPosition = vertexgen_quad_normal();
    vec2 screenPos = inSectorPos.xy - vec2(10) + inPosition.xy * (inSectorPos.zw + vec2(20));
    vec2 rectCenter = geom_rectCenterPos(inSectorPos);
    vec3 localPos = vec3(screenPos - rectCenter, 0.0);

    gl_Position = vec4(geom_toNdc(rectCenter + rotation_quat(inSectorRotation, localPos).xy, ubo.width, ubo.height),
                       inSectorDepth, 1.0);
    secColor = inSectorColor;
    secPosition = screenPos;
    secSectorPosition = inSectorPos;
    secSectorAngle = vec2(inSectorBeginAngle, inSectorEndAngle);
    secSectorFactor = inSectorFactor;
}
#endif

#ifdef FRAGMENT_SHADER
#include "basics/sdf/sdf_sector.glsl"

layout(location = 0) in vec4 secColor;
layout(location = 1) in vec2 secPosition;
layout(location = 2) in vec4 secSectorPosition;
layout(location = 3) in vec2 secSectorAngle;
layout(location = 4) in float secSectorFactor;

layout(location = 0) out vec4 outColor;

void main()
{
    vec2 sectorCenter = geom_rectCenterPos(secSectorPosition);
    vec2 halfSize = secSectorPosition.zw / 2.0;

    outColor = secColor * sdf_sector(secPosition, halfSize, sectorCenter, secSectorAngle, 0, secSectorFactor);
}
#endif