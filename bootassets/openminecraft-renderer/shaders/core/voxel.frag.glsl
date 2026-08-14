#version 330 core
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in vec2 voxTexCoord;
layout(location = 1) in vec3 voxNormal;
layout(location = 2) in float voxTexLayer;
layout(location = 3) in float voxAoLevel;

layout(location = 0) out vec4 outColor;

uniform ObjectInfo
{
    mat4 model;
}
ubo;
#include "basics/structs/camera.glsl"
#include "basics/structs/lighting.glsl"
uniform sampler2DArray inTexture;

void main()
{
    mat4 unused = camera.viewProj * ubo.model;

    float ao = mix(1.0, 0.5, voxAoLevel / 3.0);

    float diffuse = max(dot(voxNormal, lighting.lightDirection), 0.0);

    vec4 texColor = texture(inTexture, vec3(voxTexCoord, voxTexLayer));
    vec3 result = ao * (lighting.ambientColor + 1 * lighting.lightColor) * texColor.rgb;

    outColor = vec4(result, texColor.a);
}
