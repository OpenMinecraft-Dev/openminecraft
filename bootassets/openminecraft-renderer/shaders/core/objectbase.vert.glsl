#version 330 core
#extension GL_ARB_separate_shader_objects : enable

#vertex

layout(location = 0) out vec2 objTexCoord;
layout(location = 1) out vec3 objNormal;

uniform ObjectInfo
{
    mat4 model;
}
ubo;
#include "basics/structs/camera.glsl"

void main()
{
    gl_Position = camera.viewProj * ubo.model * vec4(inPosition, 1.0);
    objTexCoord = inTexCoord;
    objNormal = (ubo.model * vec4(inNormal, 1.0)).xyz;
}
