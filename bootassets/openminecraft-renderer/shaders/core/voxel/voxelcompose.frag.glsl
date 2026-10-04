#version 330 core
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in vec2 biltTexCoord;
layout(location = 0) out vec4 outColor;

uniform sampler2D inTextureCutout;
uniform sampler2D inTextureTranslucent;
uniform sampler2D inTextureTranslucentReveal;

void main()
{
    vec4 cutoutSample = texture(inTextureCutout, biltTexCoord);
    vec4 translucentSample = texture(inTextureTranslucent, biltTexCoord);
    vec4 revealSample = texture(inTextureTranslucentReveal, biltTexCoord);

    vec3 opaque = cutoutSample.rgb;
    float reveal = revealSample.r;
    vec3 accum = translucentSample.rgb;

    vec3 linearColor = accum + opaque * reveal;

    linearColor = linearColor / (linearColor + vec3(1.0));

    outColor = vec4(linearColor, 1.0);
}