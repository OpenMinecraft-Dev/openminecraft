#version 330 core
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in vec2 outTexCoord;
layout(location = 0) out vec4 outColor;

uniform sampler2D inTexture;

void main() {
    outColor = texture(inTexture, outTexCoord);
}
