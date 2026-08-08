#version 330 core
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in vec2 inTexCoord;
layout(location = 1) in vec3 inNormal;
layout(location = 0) out vec4 outColor;

uniform ObjectInfo {
    mat4 model;
    mat4 viewProj;
    vec3 lightDirection;
    vec3 lightColor;
    vec3 ambientColor;
} ubo;
uniform sampler2D inTexture;

void main() {
    float diff = max(dot(inNormal, ubo.lightDirection), 0.0);
    vec3 diffuse = diff * ubo.lightColor;

    vec4 texColor = texture(inTexture, inTexCoord);
    vec3 result = (ubo.ambientColor + diffuse) * texColor.rgb;

    outColor = vec4(result, texColor.a);
}
