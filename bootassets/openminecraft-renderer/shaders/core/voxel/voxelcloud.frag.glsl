#version 330 core
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) out vec4 outColor;
layout(location = 1) out vec4 outReveal;

#include "basics/fog.glsl"

#include "basics/depthweight.glsl"

#include "basics/structs/camera.glsl"
uniform CloudData
{
    vec3 modPos;
    float opacity;
    vec3 color;
}
cloud;
#include "basics/structs/fog.glsl"

layout(location = 0) flat in float colorMod;

void main()
{
    mat4 unused = camera.viewProj;
    float unused2 = cloud.modPos.x;

    vec4 result =
        vec4(cloud.color * cloud.opacity * colorMod * depthweight_weight(), cloud.opacity * depthweight_weight());
    outColor =
        vec4(fog_gen(vec4(result.rgb, 1.0), fog.fogStart, fog.fogEnd, vec3(fog.fogR, fog.fogG, fog.fogB), 0.0).rgb *
                 result.a * depthweight_weight(),
             result.a * depthweight_weight());
    outReveal = vec4(cloud.opacity);
}