#version 330 core
#extension GL_ARB_separate_shader_objects : enable

#include "basics/geometry.glsl"
#include "basics/rotation.glsl"

layout(location = 0) in vec2 inPosition;
layout(location = 1) in vec4 inRectPos;
layout(location = 2) in vec4 inRectColor;
layout(location = 3) in vec4 inRectRadius;
layout(location = 4) in vec4 inRectRotation;
layout(location = 5) in float inRectFactor;
layout(location = 6) in float inRectDepth;

layout(location = 0) out vec4 outColor;
layout(location = 1) out vec2 outPosition;
layout(location = 2) out vec4 outRadius;
layout(location = 3) out vec4 outRectPosition;
layout(location = 4) out float outFactor;

uniform ScreenData {
    float width;
    float height;
} ubo;

void main() {
    vec2 screenPos = inRectPos.xy - vec2(10) + inPosition.xy * (inRectPos.zw + vec2(20));
    vec2 rectCenter = geom_rectCenterPos(inRectPos);
    vec3 localPos = vec3(screenPos - rectCenter, 0.0);

    gl_Position = vec4(geom_toNdc(rectCenter + rotation_quat(inRectRotation, localPos).xy, ubo.width, ubo.height), inRectDepth, 1.0);
    outColor = inRectColor;
    outPosition = screenPos;
    outRadius = inRectRadius;
    outRectPosition = inRectPos;
    outFactor = inRectFactor;
}
