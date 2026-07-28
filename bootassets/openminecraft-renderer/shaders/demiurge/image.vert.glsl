#version 410 core
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

layout(location = 0) in vec2 inPosition;
layout(location = 1) in vec4 inRectPos;
layout(location = 2) in vec4 inRectColor;
layout(location = 3) in vec4 inRectRadius;
layout(location = 4) in float inRectFactor;
layout(location = 5) in float inRectDepth;
layout(location = 6) in float inFillType;

layout(location = 0) out vec4 outColor;
layout(location = 1) out vec2 outPosition;
layout(location = 2) out vec4 outRadius;
layout(location = 3) out vec4 outRectPosition;
layout(location = 4) out float outFactor;
layout(location = 5) out vec2 outUv;
layout(location = 6) flat out float outFillType;

layout(binding = 0) uniform UniformBufferObject {
    float width;
    float height;
} ubo;

void main() {
    vec2 screenPos = inRectPos.xy - vec2(10) + inPosition.xy * (inRectPos.zw + vec2(20));

    float ndcX = (screenPos.x / ubo.width) * 2.0 - 1.0;
    float ndcY = 1.0 - (screenPos.y / ubo.height) * 2.0;
    
    gl_Position = vec4(ndcX, ndcY, inRectDepth, 1.0);
#ifdef VULKAN
    gl_Position.y = -gl_Position.y;
#endif
    outColor = inRectColor;
    outPosition = screenPos;
    outRadius = inRectRadius;
    outRectPosition = inRectPos;
    outFactor = inRectFactor;
    outUv = inPosition;
    outFillType = inFillType;
}
