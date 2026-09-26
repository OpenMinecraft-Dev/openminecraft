#version 330 core
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in vec2 voxTexCoord;
layout(location = 1) in vec3 voxNormal;
layout(location = 2) in float voxTexLayer;
layout(location = 3) in vec2 voxLight;
layout(location = 4) in float voxFactor;
layout(location = 5) flat in int voxSec;

layout(location = 0) out vec4 outColor;

#include "basics/fog.glsl"
#include "basics/structs/camera.glsl"
uniform sampler2DArray inTexture;
uniform sampler2DArray inTextureSec;
uniform samplerBuffer inChunkPos;
#include "basics/structs/fog.glsl"
uniform sampler2D inLightmap;

void main()
{
    mat4 unused = camera.viewProj;
    float unused2 = texelFetch(inChunkPos, 0).r * texture(inTextureSec, vec3(0.0)).x;

    vec4 texColor;
    if (voxSec == 1)
    {
        texColor = texture(inTextureSec, vec3(voxTexCoord, voxTexLayer));
    }
    else
    {
        texColor = texture(inTexture, vec3(voxTexCoord, voxTexLayer));
    }

    if (texColor.a < 0.005)
    {
        discard;
    }
    vec3 result = voxFactor * texColor.rgb;
    result.r *= 0.2;
    result.g *= 0.2;

    outColor = fog_gen(vec4(result.rgb * texColor.a, texColor.a) * texture(inLightmap, voxLight), fog.fogStart,
                       fog.fogEnd, vec3(fog.fogR, fog.fogG, fog.fogB), 0.0);
}
