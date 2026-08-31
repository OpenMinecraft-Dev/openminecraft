#version 330 core
#extension GL_ARB_separate_shader_objects : enable

#include "basics/vertexgen.glsl"

#include "basics/structs/camera.glsl"
uniform SkyDiscData
{
    vec3 diskCenterColor;
    float discRange;
    vec3 diskSideColor;
    float discHeight;
}
disc;
layout(location = 0) out vec3 discColor;

#define PI 3.1415926535897

void main()
{
    vec4 actualPos;
    int id = vertexgen_id();
    if (id % 3 == 0)
    {
        actualPos = vec4(0.0, 0.0, 0.0, 1.0);
        discColor = disc.diskCenterColor;
    }
    else if (id % 3 == 1)
    {
        actualPos = vec4(sin(PI / 4 * (id / 3)), 0.0, cos(PI / 4 * (id / 3)), 1.0);
        discColor = disc.diskSideColor;
    }
    else
    {
        actualPos = vec4(sin(PI / 4 * (id / 3 + 1)), 0.0, cos(PI / 4 * (id / 3 + 1)), 1.0);
        discColor = disc.diskSideColor;
    }

    actualPos *= disc.discRange;
    if (id % 3 == 0)
    {
        actualPos.y = disc.discHeight;
    }
    else
    {
        actualPos.y = -disc.discHeight;
    }
    gl_Position = camera.viewProj * actualPos;
    gl_Position.z = 0;
}
