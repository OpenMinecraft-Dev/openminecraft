#version 330 core
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in vec2 texCoord;
layout(location = 1) flat in int texIndex;
layout(location = 0) out vec4 outColor;

#include "basics/structs/camera.glsl"
uniform MoonData
{
    float moonAngle;
    int moonPhase;
}
moon;
uniform sampler2DArray inTexture;

void main()
{
    mat4 unused = camera.viewProj;
    float unused2 = moon.moonAngle;
    outColor = texture(inTexture, vec3(texCoord, float(texIndex)));
}
