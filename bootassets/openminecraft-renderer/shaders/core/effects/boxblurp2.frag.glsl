#version 330 core
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in vec2 outTexCoord;
layout(location = 0) out vec4 outColor;

uniform BlurArgs
{
    float radius;
}
ubo;
uniform sampler2D inTextureFg;
uniform sampler2D inTexture;

void main()
{
    vec4 fg = texture(inTextureFg, outTexCoord);
    if (fg.a < 0.01)
    {
        discard;
    }

    precision highp float;

    vec2 texSize = textureSize(inTexture, 0);
    vec2 texelSize = 1.0 / texSize;

    int radius = int(ubo.radius);

    vec4 result = vec4(0.0);
    int count = 0;
    for (int x = -radius; x <= radius; x++)
    {
        vec2 offset = vec2(float(x), 0) * texelSize;
        result += texture(inTexture, outTexCoord + offset);
        count++;
    }
    vec4 o = result / float(count);

    outColor = fg.a * fg + (1 - fg.a) * o;
    outColor.a = 1.0f;
}
