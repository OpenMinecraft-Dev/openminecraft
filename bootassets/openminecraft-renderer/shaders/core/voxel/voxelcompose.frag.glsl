#version 330 core
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in vec2 biltTexCoord;
layout(location = 0) out vec4 outColor;

uniform sampler2D inTextureCutout;
uniform sampler2D inTextureTranslucent;

void main()
{
    vec3 opaque = texture(inTextureCutout, biltTexCoord).rgb;
    float reveal = texture(inTextureTranslucent, biltTexCoord).a;
    vec3 accum = texture(inTextureTranslucent, biltTexCoord).rgb;

    vec3 limited_accum = accum / (vec3(2.0) + accum);

    vec3 linearColor = opaque * reveal + limited_accum * (1 - reveal);

    outColor = vec4(linearColor, 1.0);
}