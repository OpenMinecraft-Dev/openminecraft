#version 330 core
#extension GL_ARB_separate_shader_objects : enable

#include "basics/geometry.glsl"
#include "basics/rotation.glsl"

layout(location = 0) in vec2 inPosition;
layout(location = 1) in vec4 inSectorPos;
layout(location = 2) in vec4 inSectorColor;
layout(location = 3) in vec4 inSectorRotation;
layout(location = 4) in float inSectorBeginAngle;
layout(location = 5) in float inSectorEndAngle;
layout(location = 6) in float inSectorFactor;
layout(location = 7) in float inSectorDepth;

layout(location = 0) out vec4 secColor;
layout(location = 1) out vec2 secPosition;
layout(location = 2) out vec4 secSectorPosition;
layout(location = 3) out vec2 secSectorAngle;
layout(location = 4) out float secSectorFactor;

uniform ScreenData {
    float width;
    float height;
} ubo;

void main() {
    vec2 screenPos = inSectorPos.xy - vec2(10) + inPosition.xy * (inSectorPos.zw + vec2(20));
    vec2 rectCenter = geom_rectCenterPos(inSectorPos);
    vec3 localPos = vec3(screenPos - rectCenter, 0.0);

    gl_Position = vec4(geom_toNdc(rectCenter + rotation_quat(inSectorRotation, localPos).xy, ubo.width, ubo.height), inSectorDepth, 1.0);
    secColor = inSectorColor;
    secPosition = screenPos;
    secSectorPosition = inSectorPos;
    secSectorAngle = vec2(inSectorBeginAngle, inSectorEndAngle);
    secSectorFactor = inSectorFactor;
}
