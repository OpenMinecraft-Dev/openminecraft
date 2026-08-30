#ifdef VERTEX_SHADER
#include "basics/vertexgen.glsl"

layout(location = 0) out vec2 outTexCoord;

void main()
{
    gl_Position = vec4(vertexgen_quad_ndc(), 0.0, 1.0);
    outTexCoord = vertexgen_quad_normal();
}
#endif

#ifdef FRAGMENT_SHADER
layout(location = 0) in vec2 outTexCoord;
layout(location = 0) out vec4 outColor;

uniform BlurArgs
{
    float radius;
    int blurType;
    float direction;
}
ubo;
uniform sampler2D inTexture;

#define MAX_KERNEL_RADIUS 16384
#define EPS 0.0001
float gaussianWeight(float x, float sigma)
{
    return exp(-(x * x) / (2.0 * sigma * sigma));
}

void main()
{
    precision highp float;

    vec2 texSize = textureSize(inTexture, 0);
    vec2 texelSize = 1.0 / texSize;

    float sigma = ubo.radius / 3.0;
    int radius = int(ubo.radius);

    vec4 result = vec4(0.0);
    float weightSum = 0.0;

    for (int x = -radius; x <= radius; x++)
    {
        float weight;
        switch (ubo.blurType)
        {
        default:
        case 0:
            weight = exp(-(x * x) / (2.0 * sigma * sigma));
            break;
        case 1:
            weight = 1.0;
            break;
        case 2:
            weight = 1.0 - (float(abs(x)) / float(radius));
            break;
        case 3:
            weight = 1.0 - smoothstep(0.0, 1.0, abs(x) / radius);
            break;
        case 4:
            weight = 1.0 / (1.0 + (x * x / sigma / sigma));
            break;
        }

        float dir = clamp(ubo.direction, 0.0, 1.0);
        vec2 off = outTexCoord + vec2(dir * x, (1 - dir) * x) * texelSize;
        // gino: we had to do this otherwise wrong pixels will be fetched
        off = clamp(off, vec2(0.0 + texelSize), vec2(1.0 - texelSize));
        vec4 sampleColor = texture(inTexture, off);

        result += sampleColor * weight;
        weightSum += weight;
    }

    outColor = result / weightSum;
}
#endif