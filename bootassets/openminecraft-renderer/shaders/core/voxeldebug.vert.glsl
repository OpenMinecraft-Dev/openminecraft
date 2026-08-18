#version 330 core
#extension GL_ARB_separate_shader_objects : enable

#vertex

#include "basics/vertexgen.glsl"

#include "basics/structs/camera.glsl"

void main()
{
    gl_Position = camera.viewProj * vec4(voxelPos, 1.0);
}