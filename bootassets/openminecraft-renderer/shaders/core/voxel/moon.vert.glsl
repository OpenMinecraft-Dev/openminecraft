#version 330 core
#extension GL_ARB_separate_shader_objects : enable

#include "basics/vertexgen.glsl"

#include "basics/structs/camera.glsl"
uniform MoonData
{
    float moonAngle;
    int moonPhase;
}
moon;
layout(location = 0) out vec2 texCoord;
layout(location = 1) flat out int texIndex;

void main()
{
    vec2 p = vertexgen_quad_ndc();
    vec3 pos = vec3(p.x * 16, 100.0, p.y * 16);

    float ca = cos(radians(moon.moonAngle));
    float sa = sin(radians(moon.moonAngle));

    mat3 rotY = mat3(0.0, 0.0, 1.0, 0.0, 1.0, 0.0, -1.0, 0.0, 0.0);
    mat3 rotX = mat3(1.0, 0.0, 0.0, 0.0, ca, sa, 0.0, -sa, ca);
    mat3 combined = rotY * rotX;

    gl_Position = camera.viewProj * vec4(combined * pos, 1.0);
    texCoord = vertexgen_quad_normal();
    texIndex = moon.moonPhase;
}
