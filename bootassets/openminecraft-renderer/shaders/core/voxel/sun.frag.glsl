#version 330 core
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in vec2 texCoord;
layout(location = 0) out vec4 outColor;

#include "basics/structs/camera.glsl"
uniform SunRiseData
{
    vec4 color;
    float sunAngle;
}
sun;
uniform sampler2D inTexture;

void main()
{
    mat4 unused = camera.viewProj;
    float unused2 = sun.sunAngle;
    outColor = texture(inTexture, texCoord);
}
