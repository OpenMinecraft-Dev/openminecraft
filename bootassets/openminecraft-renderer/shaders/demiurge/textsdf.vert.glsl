#version 410 core
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

layout(location = 0) in vec2 inPosition;
layout(location = 1) in vec4 inTextPos;
layout(location = 2) in vec4 inTextColor;
layout(location = 3) in float inTextDepth;
layout(location = 4) in float inTextHeight;
layout(location = 5) in float inTextFactor;
layout(location = 6) in int inTextGlyphId;

layout(location = 0) out vec4 outColor;

layout(binding = 0) uniform UniformBufferObject {
    float width;
    float height;
} ubo;

void main() {
    vec2 screenPos = inTextPos.xy + inPosition.xy * inTextPos.zw * inTextHeight;

    float ndcX = (screenPos.x / ubo.width) * 2.0 - 1.0;
    float ndcY = 1.0 - (screenPos.y / ubo.height) * 2.0;
    
    gl_Position = vec4(ndcX, ndcY, inTextDepth, 1.0);
#ifdef VULKAN
    gl_Position.y = -gl_Position.y;
#endif
    outColor = inTextColor;
}
