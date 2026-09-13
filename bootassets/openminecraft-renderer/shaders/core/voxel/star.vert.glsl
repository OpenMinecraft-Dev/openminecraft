#version 330 core
#extension GL_ARB_separate_shader_objects : enable

#vertex

#include "basics/vertexgen.glsl"

#include "basics/structs/camera.glsl"

mat3 billboardRotation(vec3 center, float zRot)
{
    vec3 toCamera = normalize(-center);
    vec3 worldUp = (abs(toCamera.y) > 0.99) ? vec3(1.0, 0.0, 0.0) : vec3(0.0, 1.0, 0.0);

    vec3 right = normalize(cross(worldUp, toCamera));
    vec3 up = cross(toCamera, right);

    mat3 look = mat3(right, up, toCamera);

    float c = cos(zRot);
    float s = sin(zRot);
    mat3 spin = mat3(c, s, 0.0, -s, c, 0.0, 0.0, 0.0, 1.0);

    return look * spin;
}

void main()
{
    vec2 p = vertexgen_quad_ndc();
    vec3 local = vec3(p * starSize, 0.0);

    local = billboardRotation(starCenter, starZrot) * local + starCenter;

    gl_Position = camera.viewProj * vec4(local, 1.0);
}
