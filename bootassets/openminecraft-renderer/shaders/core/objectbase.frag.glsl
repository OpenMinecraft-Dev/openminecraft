#version 330 core
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in vec2 objTexCoord;
layout(location = 1) in vec3 objNormal;
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
    float diff = max(dot(objNormal, lighting.lightDirection), 0.0);
    vec3 diffuse = diff * lighting.lightColor;

    vec4 texColor = texture(inTexture, vec3(objTexCoord, 0));
    vec3 result = (lighting.ambientColor + diffuse) * texColor.rgb;

    outColor = vec4(result, texColor.a);
}
