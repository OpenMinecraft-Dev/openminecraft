#version 450 core
#extension GL_ARB_separate_shader_objects : enable

#include "basics/geometry.glsl"

layout(location = 0) in vec2 inPosition;
layout(location = 1) in vec4 inRectPos;
layout(location = 2) in vec4 inRectColor;
layout(location = 3) in float inRectDepth;

layout(location = 0) out vec4 outColor;

layout(binding = 0) uniform UniformBufferObject {
    float width;
    float height;
} ubo;

void main() {
    vec2 screenPos = inRectPos.xy + inPosition.xy * inRectPos.zw;

    gl_Position = vec4(geom_toNdc(screenPos, ubo.width, ubo.height), inRectDepth, 1.0);
    outColor = inRectColor;
}
