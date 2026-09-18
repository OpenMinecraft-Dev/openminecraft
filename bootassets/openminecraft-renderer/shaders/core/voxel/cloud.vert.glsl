#version 330 core
#extension GL_ARB_separate_shader_objects : enable

#include "basics/vertexgen.glsl"

#include "basics/structs/camera.glsl"
uniform CloudData
{
    vec3 modPos;
}
cloud;

layout(location = 0) out vec2 cloudPos;

void main()
{
    vec2 pl = vertexgen_quad_ndc() * 256.0 * 12.0 - cloud.modPos.xz + vec2(128.0 * 12.0, 128.0 * 12.0);
    gl_Position = camera.viewProj * vec4(vec3(pl.x, 192 - cloud.modPos.y, pl.y), 1.0);

    cloudPos = vertexgen_quad_ndc();
}
