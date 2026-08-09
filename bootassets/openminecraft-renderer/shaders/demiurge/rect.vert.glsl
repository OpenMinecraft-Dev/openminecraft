#version 330 core
#extension GL_ARB_separate_shader_objects : enable

#include "basics/geometry.glsl"

#vertex

layout(location = 0) out vec4 rectColor;

uniform ScreenData {
    float width;
    float height;
} ubo;

void main() {
    vec2 screenPos = inRectPos.xy + inPosition.xy * inRectPos.zw;

    gl_Position = vec4(geom_toNdc(screenPos, ubo.width, ubo.height), inRectDepth, 1.0);
    rectColor = inRectColor;
}
