#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in vec2 outTexCoord;
layout(location = 0) out vec4 outColor;

layout(binding = 1) uniform sampler2D texSampler;

layout(binding = 0) uniform UniformBufferObject {
    mat4 model;
    mat4 view;
    mat4 proj;
    float kernelSize;
} ubo;

#define MAX_KERNEL_RADIUS 10
float gaussianWeight(float x, float y, float sigma) {
    float sigma2 = sigma * sigma;
    return exp(-(x * x + y * y) / (2.0 * sigma2));
}

void main() {
    outColor = texture(texSampler, outTexCoord);
    
    vec2 texSize = textureSize(texSampler, 0);
    vec2 texelSize = 1.0 / texSize;

    int radius = 9;
    if (radius <= 0) {
        radius = int(ceil(3.0 * 3));
    }
    radius = min(radius, MAX_KERNEL_RADIUS);
    
    vec4 result = vec4(0.0);
    float weightSum = 0.0;
    
    for (int y = -radius; y <= radius; y++) {
        for (int x = -radius; x <= radius; x++) {
            float weight = gaussianWeight(float(x), float(y), 3);
            
            vec2 off = outTexCoord + vec2(x, y) * texelSize;
            off = max(min(off, vec2(1.0)), vec2(0.0));
            vec4 sampleColor = texture(texSampler, off);
            
            result += sampleColor * weight;
            weightSum += weight;
        }
    }
    
    outColor = result / weightSum;
}
