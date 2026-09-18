#version 330 core
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in vec2 cloudPos;
layout(location = 0) out vec4 outColor;

#include "basics/structs/camera.glsl"
uniform CloudData
{
    vec3 modPos;
}
cloud;
uniform sampler2D inTexture;

void main()
{
    mat4 unused = camera.viewProj;
    float unused2 = cloud.modPos.x;
    outColor = texture(inTexture, cloudPos) * 0.9;
}