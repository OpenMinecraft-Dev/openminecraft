#ifndef FOG_GLSL
#define FOG_GLSL

#include "basics/structs/camera.glsl"

vec4 fog_gen(vec4 original, float fogStart, float fogEnd, vec3 fogColor, float targetOpacity)
{
    float d = gl_FragCoord.z;

#ifndef VULKAN
    d = (d - 0.5) * 2;
#endif

    float near = max(camera.nearFar.x, 0.0001);
    float far = max(camera.nearFar.y, near + 0.0001);
    d = (near * far) / (far - d * (far - near));

    // FogData may be supplied as either (start, end) or (end, start); the
    // smaller value is the real world-space start distance.
    float start = min(fogStart, fogEnd);
    float end = max(fogStart, fogEnd);
    float t = end > start ? clamp((d - start) / (end - start), 0.0, 1.0) : step(end, d);

    return vec4(mix(original.rgb, fogColor, t), mix(original.a, targetOpacity, t));
}

vec4 fog_gendefault(vec4 original, float targetOpacity)
{
    return fog_gen(original, 0.0005, 0.0006, vec3(0.470, 0.654, 1.0), targetOpacity);
}

#endif