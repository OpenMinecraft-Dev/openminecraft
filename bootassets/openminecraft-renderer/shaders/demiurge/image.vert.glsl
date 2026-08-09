#version 330 core
#extension GL_ARB_separate_shader_objects : enable

#include "basics/geometry.glsl"
#include "basics/rotation.glsl"

#vertex

layout(location = 0) out vec4 imageColor;
layout(location = 1) out vec2 imagePosition;
layout(location = 2) out vec4 imageRadius;
layout(location = 3) out vec4 imageRectPosition;
layout(location = 4) out float imageFactor;
layout(location = 5) out vec2 iamgeUv;
layout(location = 6) flat out float imageFillType;

uniform ScreenData {
    float width;
    float height;
} ubo;

void main() {
    vec2 screenPos = inRectPos.xy - vec2(10) + inPosition.xy * (inRectPos.zw + vec2(20));
    vec2 rectCenter = geom_rectCenterPos(inRectPos);
    vec3 localPos = vec3(screenPos - rectCenter, 0.0);

    gl_Position = vec4(geom_toNdc(rectCenter + rotation_quat(inRectRotation, localPos).xy, ubo.width, ubo.height), inRectDepth, 1.0);
    imageColor = inRectColor;
    imagePosition = screenPos;
    imageRadius = inRectRadius;
    imageRectPosition = inRectPos;
    imageFactor = inRectFactor;
    imageUv = inPosition;
    imageFillType = inFillType;
}
