#ifndef DEPTHWEIGHT_GLSL
#define DEPTHWEIGHT_GLSL

float depthweight_getDepth()
{
    float d = gl_FragCoord.z;

#ifndef VULKAN
    d = (d - 0.5) * 2;
#endif

    return d;
}

float depthweight_weight()
{
    return clamp(pow(1.0 - depthweight_getDepth(), 3.0) * 1e3, 1e-2, 3e3);
}

#endif