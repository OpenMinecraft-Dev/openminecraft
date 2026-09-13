#version 330 core
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) out vec4 outColor;

#include "basics/structs/camera.glsl"

void main()
{
    outColor = vec4(vec3(1.0), 0.4);
}
