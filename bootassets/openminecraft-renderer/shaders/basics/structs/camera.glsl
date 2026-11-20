#ifndef CAMERA_STRUCT_GLSL
#define CAMERA_STRUCT_GLSL

uniform Camera
{
    mat4 viewProj;
    vec4 nearFar;
}
camera;

#endif
