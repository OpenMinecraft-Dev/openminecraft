#version 330 core
#extension GL_ARB_separate_shader_objects : enable

#include "basics/vertexgen.glsl"

#include "basics/structs/camera.glsl"
layout(location = 0) out vec3 discColor;

#define PI 3.1415926535897

void main()
{
    vec4 actualPos;
    int id = vertexgen_id();
    if (id % 3 == 0)
    {
        actualPos = vec4(0.0, 0.0, 0.0, 1.0);
        discColor = vec3(0.470, 0.654, 1.0);
    }
    else if (id % 3 == 1)
    {
        actualPos = vec4(sin(PI / 4 * (id / 3)), 0.0, cos(PI / 4 * (id / 3)), 1.0);
        discColor = vec3(0.198, 0.371, 1.0);
    }
    else
    {
        actualPos = vec4(sin(PI / 4 * (id / 3 + 1)), 0.0, cos(PI / 4 * (id / 3 + 1)), 1.0);
        discColor = vec3(0.198, 0.371, 1.0);
    }

    actualPos *= 256;
    actualPos.y = -16;
    gl_Position = camera.viewProj * actualPos;
}