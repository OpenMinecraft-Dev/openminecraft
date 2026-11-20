#version 330 core
#extension GL_ARB_separate_shader_objects : enable

#include "basics/vertexgen.glsl"

#include "basics/structs/camera.glsl"
uniform SunRiseData
{
    vec4 color;
    float sunAngle;
}
sun;
layout(location = 0) out vec2 texCoord;

void main()
{
    vec2 p = vertexgen_quad_ndc();
    vec3 pos = vec3(p.x * 16, 100.0, p.y * 16);

    float ca = cos(radians(sun.sunAngle));
    float sa = sin(radians(sun.sunAngle));

    mat3 rotY = mat3(0.0, 0.0, 1.0, 0.0, 1.0, 0.0, -1.0, 0.0, 0.0);
    mat3 rotX = mat3(1.0, 0.0, 0.0, 0.0, ca, sa, 0.0, -sa, ca);
    mat3 combined = rotY * rotX;

    gl_Position = camera.viewProj * vec4(combined * pos, 1.0);
    texCoord = vertexgen_quad_normal();
}
