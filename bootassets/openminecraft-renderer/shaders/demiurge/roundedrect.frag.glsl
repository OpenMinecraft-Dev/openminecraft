#version 410 core
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

#include "basics/sdf_rrect.glsl"

layout(location = 0) in vec4 inColor;
layout(location = 1) in vec2 inPosition;
layout(location = 2) in vec4 inRadius;
layout(location = 3) in vec4 inRectPosition;
layout(location = 4) in float inFactor;

layout(location = 0) out vec4 outColor;

void main() {
    vec2 rectCenter = inRectPosition.xy + inRectPosition.zw / 2.0;
    vec2 halfSize   = inRectPosition.zw / 2.0;
    vec2 localPos   = inPosition - rectCenter;

    float alpha = rrectSdf(localPos, halfSize, inRadius, 0.0, inFactor);

    outColor = inColor * alpha;
}
