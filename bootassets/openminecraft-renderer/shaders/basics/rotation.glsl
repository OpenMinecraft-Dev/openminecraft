#ifndef ROTATION_GLSL
#define ROTATION_GLSL

vec3 rotation_quat(vec4 q, vec3 v)
{
    vec3 t = 2.0 * cross(q.xyz, v);
    return v + q.w * t + cross(q.xyz, t);
}

#endif
