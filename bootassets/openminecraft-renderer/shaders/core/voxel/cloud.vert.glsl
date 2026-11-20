#version 330 core
#extension GL_ARB_separate_shader_objects : enable

#include "basics/vertexgen.glsl"

layout(location = 0) out vec2 cloudPos;

void main()
{
    gl_Position = vec4(vertexgen_quad_ndc(), 1.0, 1.0);

    cloudPos = vertexgen_quad_normal();
}
