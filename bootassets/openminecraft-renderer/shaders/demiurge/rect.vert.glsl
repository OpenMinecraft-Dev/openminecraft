#version 410 core
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec4 pos;
layout(location = 2) in vec4 inColor;
layout(location = 3) in float depth;

layout(location = 0) out vec4 outColor;

layout(binding = 0) uniform UniformBufferObject {
    float width;
    float height;
} ubo;

void main() {
    vec2 screenPos = pos.xy + inPosition.xy * pos.zw;

    float ndcX = (screenPos.x / ubo.width) * 2.0 - 1.0;
    float ndcY = 1.0 - (screenPos.y / ubo.height) * 2.0;
    
    gl_Position = vec4(ndcX, ndcY, depth, 1.0);
outColor = inColor;
#ifdef VULKAN
    gl_Position.y = -gl_Position.y;
#endif
    outColor = inColor;
}
