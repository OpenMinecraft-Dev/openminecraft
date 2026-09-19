#version 330 core
#extension GL_ARB_separate_shader_objects : enable

#vertex

#include "basics/vertexgen.glsl"

#include "basics/structs/camera.glsl"
uniform CloudData
{
    vec3 modPos;
}
cloud;

void main()
{
    vec2 pl = vertexgen_quad_normal() * 12.0;
    int ox = ((cloudInfo >> 8) & 0xff) - 127;
    int oy = (cloudInfo & 0xff) - 127;

    vec2 loc = vec2(ox * 12.0, oy * 12.0);

    loc -= cloud.modPos.xz;
    loc = mod(loc, vec2(256.0 * 12.0)) - vec2(128.0 * 12.0);

    pl += loc;

    gl_Position = camera.viewProj * vec4(vec3(pl.x, 192 - cloud.modPos.y, pl.y), 1.0);
}
