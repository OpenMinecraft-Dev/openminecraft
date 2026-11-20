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
    vec4 accumSample = texture(inTextureTranslucent, biltTexCoord);
    float reveal = texture(inTextureTranslucentReveal, biltTexCoord).r;

    vec3 opaque = cutoutSample.rgb;

    vec3 averageColor = accumSample.rgb / max(accumSample.a, 1e-5);

    vec3 background = opaque * reveal;

    vec3 linearColor = averageColor + background;

    linearColor = linearColor / (linearColor + vec3(1.0));

    outColor = vec4(linearColor, 1.0);
}