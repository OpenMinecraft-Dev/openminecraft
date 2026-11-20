#version 330 core
#extension GL_ARB_separate_shader_objects : enable

#include "basics/vertexgen.glsl"

void main()
{
    gl_Position = vec4(vertexgen_quad_ndc(), 0.0, 1.0);
}
