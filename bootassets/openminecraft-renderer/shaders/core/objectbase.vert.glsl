#version 450 core
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec2 inTexCoord;
layout(location = 2) in vec3 inNormal;

layout(location = 0) out vec2 outTexCoord;
layout(location = 1) out vec3 outNormal;

layout(binding = 0) uniform UniformBufferObject {
    mat4 model;
    mat4 view;
    mat4 proj;
    vec3 lightDirection;
    vec3 lightColor;
    vec3 ambientColor;
} ubo;

void main() {
    gl_Position = ubo.proj * ubo.view * ubo.model * vec4(inPosition, 1.0);
    outTexCoord = inTexCoord;
    outNormal = (ubo.model * vec4(inNormal, 1.0)).xyz;
}
