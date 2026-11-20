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
layout(location = 0) out vec4 discColor;

#define PI 3.1415926535897

void main()
{
    vec4 actualPos;
    int id = vertexgen_id();

    if (id == 0)
    {
        actualPos = vec4(0.0, 100.0, 0.0, 1.0);
        discColor = sun.color;
    }
    else
    {
        float angle = (id * 2 * PI) / 16.0f;
        float sinAngle = sin(angle);
        float cosAngle = cos(angle);
        actualPos = vec4(sinAngle * 120.0, cosAngle * 120.0, (-cosAngle) * 40.0, 1.0);
        discColor = sun.color * vec4(vec3(1.0), 0.0);
    }

    mat3 rotX90 = mat3(1.0, 0.0, 0.0, 0.0, 0.0, 1.0, 0.0, -1.0, 0.0);
    float angleZ = (sin(radians(sun.sunAngle)) < 0.0) ? 180.0 : 0.0;
    float zRad = radians(angleZ + 90.0);
    float cz = cos(zRad);
    float sz = sin(zRad);
    mat3 rotZ = mat3(cz, sz, 0.0, -sz, cz, 0.0, 0.0, 0.0, 1.0);
    mat3 scaleM = mat3(1.0, 0.0, 0.0, 0.0, 1.0, 0.0, 0.0, 0.0, sun.color.a);
    mat3 combined = rotX90 * rotZ * scaleM;

    actualPos.xyz = combined * actualPos.xyz;
    gl_Position = camera.viewProj * actualPos;
    gl_Position.z = 0;
}
