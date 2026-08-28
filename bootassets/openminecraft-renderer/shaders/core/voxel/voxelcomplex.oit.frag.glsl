#version 330 core
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in vec2 voxTexCoord;
layout(location = 1) in vec3 voxNormal;
layout(location = 2) in float voxTexLayer;
layout(location = 3) in float voxAoLevel;
layout(location = 4) in vec2 voxLight;
layout(location = 5) in float voxFactor;
layout(location = 6) flat in int voxSecondaryTex;

layout(location = 0) out vec4 outColor;

#include "basics/fog.glsl"
#include "basics/structs/camera.glsl"
uniform sampler2DArray inTexture;
uniform sampler2DArray inTextureSec;

void main()
{
    mat4 unused = camera.viewProj;

    float ao = mix(1.0, 0.2, voxAoLevel / 3.0);

    vec4 texColor;
    if (voxSecondaryTex == 0)
    {
        texColor = texture(inTexture, vec3(voxTexCoord, voxTexLayer));
    }
    else
    {
        texColor = texture(inTextureSec, vec3(voxTexCoord, voxTexLayer));
    }

    if (texColor.a < 0.005)
    {
        discard;
    }
    vec3 result = voxFactor * ao * texColor.rgb;

    outColor = fog_gendefault(vec4(result.rgb * texColor.a, texColor.a), 0.0);
}
