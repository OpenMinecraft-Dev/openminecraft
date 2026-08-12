#version 330 core
#extension GL_ARB_separate_shader_objects : enable

#include "basics/vertexgen.glsl"

layout(location = 0) out vec2 objTexCoord;
layout(location = 1) out vec3 objNormal;

uniform ObjectInfo {
    mat4 model;
} ubo;
#include "basics/structs/camera.glsl"

void main() {
    vec2 or = vertexgen_quad_normal_ccw();
    gl_Position = camera.viewProj * ubo.model * vec4(or.x, 1.0, or.y, 1.0);

    objTexCoord = or;
    objNormal = normalize((ubo.model * vec4(0.0, -1.0, 0.0, 0.0)).xyz);
}
