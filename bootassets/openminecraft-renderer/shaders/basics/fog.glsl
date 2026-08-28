#ifndef FOG_GLSL
#define FOG_GLSL

vec3 fog_gen(vec3 original, float fogStart, float fogEnd, vec3 fogColor)
{
    return mix(original, fogColor, smoothstep(fogEnd, fogStart, gl_FragCoord.z));
}

vec3 fog_gendefault(vec3 original)
{
    return fog_gen(original, 0.0005, 0.0006, vec3(0.470, 0.654, 1.0));
}

#endif