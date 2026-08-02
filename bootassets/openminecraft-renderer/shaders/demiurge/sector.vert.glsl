#version 450 core
#extension GL_ARB_separate_shader_objects : enable

#include "basics/geometry.glsl"

layout(location = 0) in vec2 inPosition;
layout(location = 1) in vec4 inSectorPos;
layout(location = 2) in vec4 inSectorColor;
layout(location = 3) in float inSectorRadius;
layout(location = 4) in float inSectorBeginAngle;
layout(location = 5) in float inSectorEndAngle;
layout(location = 6) in float inSectorFactor;
layout(location = 7) in float inSectorDepth;

layout(location = 0) out vec4 outColor;
layout(location = 1) out vec2 outPosition;
layout(location = 2) out vec4 outSectorPosition;
layout(location = 3) out float outSectorRadius;
layout(location = 4) out vec2 outSectorAngle;
layout(location = 5) out float outSectorFactor;

layout(binding = 0) uniform UniformBufferObject {
    float width;
    float height;
} ubo;

void main() {
    vec2 screenPos = inSectorPos.xy - vec2(10) + inPosition.xy * (inSectorPos.zw + vec2(20));

    gl_Position = vec4(geom_toNdc(screenPos, ubo.width, ubo.height), inSectorDepth, 1.0);
#ifdef VULKAN
    gl_Position.y = -gl_Position.y;
#endif
    outColor = inSectorColor;
    outPosition = screenPos;
    outSectorPosition = inSectorPos;
    outSectorRadius = inSectorRadius;
    outSectorAngle = vec2(inSectorBeginAngle, inSectorEndAngle);
    outSectorFactor = inSectorFactor;
}
