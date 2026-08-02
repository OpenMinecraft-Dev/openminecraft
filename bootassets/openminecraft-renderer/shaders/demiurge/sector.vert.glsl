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

layout(binding = 0) uniform UniformBufferObject {
    float width;
    float height;
} ubo;

void main() {
    vec2 screenPos = inSectorPos.xy + inPosition.xy * inSectorPos.zw;

    gl_Position = vec4(geom_toNdc(screenPos, ubo.width, ubo.height), inSectorDepth, 1.0);
#ifdef VULKAN
    gl_Position.y = -gl_Position.y;
#endif
    outColor = inSectorColor;
}
