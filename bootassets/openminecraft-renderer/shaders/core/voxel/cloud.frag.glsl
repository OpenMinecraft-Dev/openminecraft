#version 330 core
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in vec2 cloudPos;
layout(location = 0) out vec4 outColor;
layout(location = 1) out vec4 outReveal;

uniform sampler2D inTexture;
uniform sampler2D inTextureReveal;

void main()
{
    outColor = texture(inTexture, cloudPos);
    outReveal = vec4(texture(inTextureReveal, cloudPos).r);
}