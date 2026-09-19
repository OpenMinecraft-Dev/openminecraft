#version 330 core
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) out vec4 outColor;

#include "basics/structs/camera.glsl"
uniform CloudData
{
    vec3 modPos;
}
cloud;

void main()
{
    mat4 unused = camera.viewProj;
    float unused2 = cloud.modPos.x;
    outColor = vec4(vec3(1.0), 0.7);
}