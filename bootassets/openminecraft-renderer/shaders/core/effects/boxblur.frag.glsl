#version 330 core
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in vec2 outTexCoord;
layout(location = 0) out vec4 outColor;

uniform BlurArgs
{
    float radius;
    vec2 direction;
}
ubo;
uniform sampler2D inTexture;

#define MAX_KERNEL_RADIUS 16384
#define EPS 0.0001
float gaussianWeight(float x, float y, float sigma)
{
    float sigma2 = sigma * sigma;
    return exp(-(x * x + y * y) / (2.0 * sigma2));
}

void main()
{
    precision highp float;

    vec2 texSize = textureSize(inTexture, 0);
    vec2 texelSize = 1.0 / texSize;

    int radius = int(ubo.radius);

    /*vec4 result = vec4(0.0);
    float weightSum = 0.0;

    for (int x = -radius; x <= radius; x++) {
        float weight = gaussianWeight(0, float(x), ubo.sigma);

        vec2 off = outTexCoord + vec2(0, x) * texelSize;
        // gino: we had to do this otherwise wrong pixels will be fetched
        off = clamp(off, vec2(0.0 + texelSize), vec2(1.0 - texelSize));
        vec4 sampleColor = texture(inTexture, off);

        result += sampleColor * weight;
        weightSum += weight;
    }

    vec4 o = result / weightSum;*/

    vec4 result = vec4(0.0);
    int count = 0;
    for (int x = -radius; x <= radius; x++)
    {
        vec2 offset = direction * float(x) * texelSize;
        result += texture(inTexture, outTexCoord + offset);
        count++;
    }
    vec4 o = result / float(count);
    outColor = o;
}
