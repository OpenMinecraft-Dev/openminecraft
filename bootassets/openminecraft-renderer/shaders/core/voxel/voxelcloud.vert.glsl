#version 330 core
#extension GL_ARB_separate_shader_objects : enable

#vertex

#include "basics/vertexgen.glsl"

#include "basics/structs/camera.glsl"
uniform CloudData
{
    vec3 modPos;
    float opacity;
    vec3 color;
}
cloud;

const vec3 faceCorners[24] = vec3[24](
    // +X
    vec3(1, 0, 0), vec3(1, 1, 0), vec3(1, 1, 1), vec3(1, 0, 1),
    // -X
    vec3(0, 0, 1), vec3(0, 1, 1), vec3(0, 1, 0), vec3(0, 0, 0),
    // +Y
    vec3(0, 1, 0), vec3(0, 1, 1), vec3(1, 1, 1), vec3(1, 1, 0),
    // -Y
    vec3(0, 0, 1), vec3(0, 0, 0), vec3(1, 0, 0), vec3(1, 0, 1),
    // +Z
    vec3(0, 0, 1), vec3(1, 0, 1), vec3(1, 1, 1), vec3(0, 1, 1),
    // -Z
    vec3(1, 0, 0), vec3(0, 0, 0), vec3(0, 1, 0), vec3(1, 1, 0));

const int triIndices[6] = int[6](0, 1, 2, 0, 2, 3);

void main()
{
    switch (vertexgen_id() / 6)
    {
    case 0:
        if (((cloudInfo >> 20) & 1) == 1)
        {
            gl_Position = vec4(0.0, 0.0, 100.0, 1.0);
            return;
        }
        break;
    case 1:
        if (((cloudInfo >> 19) & 1) == 1)
        {
            gl_Position = vec4(0.0, 0.0, 100.0, 1.0);
            return;
        }
        break;
    case 4:
        if (((cloudInfo >> 18) & 1) == 1)
        {
            gl_Position = vec4(0.0, 0.0, 100.0, 1.0);
            return;
        }
        break;
    case 5:
        if (((cloudInfo >> 17) & 1) == 1)
        {
            gl_Position = vec4(0.0, 0.0, 100.0, 1.0);
            return;
        }
        break;
    default:
        break;
    }
    int iid = vertexgen_id() / 6 * 4 + triIndices[vertexgen_id() % 6];
    vec3 pl = faceCorners[iid] * vec3(12.0, 3.0, 12.0);
    int ox = ((cloudInfo >> 8) & 0xff) - 127;
    int oy = (cloudInfo & 0xff) - 127;

    vec2 loc = vec2(ox * 12.0, oy * 12.0);

    loc -= cloud.modPos.xz;
    loc = mod(loc, vec2(256.0 * 12.0)) - vec2(128.0 * 12.0);

    pl += vec3(loc.x, 192 - cloud.modPos.y, loc.y);

    gl_Position = camera.viewProj * vec4(pl, 1.0);
}
