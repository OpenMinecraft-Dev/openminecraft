#version 330 core
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in vec3 discColor;
layout(location = 0) out vec4 outColor;

void main()
{
    outColor = vec4(discColor, 1.0);
}