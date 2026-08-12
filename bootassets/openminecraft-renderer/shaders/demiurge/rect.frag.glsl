#version 330 core
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in vec4 rectColor;
layout(location = 0) out vec4 outColor;

void main()
{
    outColor = rectColor;
}
