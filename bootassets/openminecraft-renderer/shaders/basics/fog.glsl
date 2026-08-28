#ifndef FOG_GLSL
#define FOG_GLSL

vec4 fog_gen(vec4 original, float fogStart, float fogEnd, vec3 fogColor, float targetOpacity)
{
    float d = gl_FragCoord.z;

#ifndef VULKAN
    d = (d - 0.5) * 2;
#endif

    return vec4(mix(original.rgb, fogColor, smoothstep(fogEnd, fogStart, d)),
                mix(original.a, targetOpacity, smoothstep(fogEnd, fogStart, d)));
}

vec4 fog_gendefault(vec4 original, float targetOpacity)
{
    return fog_gen(original, 0.0005, 0.0006, vec3(0.470, 0.654, 1.0), targetOpacity);
}

#endif