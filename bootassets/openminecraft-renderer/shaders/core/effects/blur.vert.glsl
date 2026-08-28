#version 330 core
#extension GL_ARB_separate_shader_objects : enable

#include "basics/vertexgen.glsl"

layout(location = 0) out vec2 outTexCoord;

void main()
{
    gl_Position = vec4(vertexgen_quad_ndc(), 0.0, 1.0);
    outTexCoord = vertexgen_quad_normal();
}
