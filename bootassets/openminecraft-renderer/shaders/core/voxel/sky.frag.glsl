#version 330 core
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) out vec4 outColor;

uniform SkyDiscData
{
    vec3 diskCenterColor;
    float discRange;
    vec3 diskSideColor;
    float discHeight;
}
disc;

void main()
{
    outColor = vec4(disc.diskSideColor, 1.0);
}
