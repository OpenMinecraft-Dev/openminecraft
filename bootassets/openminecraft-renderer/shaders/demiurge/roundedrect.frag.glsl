#version 450 core
#extension GL_ARB_separate_shader_objects : enable

#include "basics/sdf/sdf_rrect.glsl"

layout(location = 0) in vec4 inColor;
layout(location = 1) in vec2 inPosition;
layout(location = 2) in vec4 inRadius;
layout(location = 3) in vec4 inRectPosition;
layout(location = 4) in float inFactor;

layout(location = 0) out vec4 outColor;

void main() {
    vec2 rectCenter = geom_rectCenterPos(inRectPosition);
    vec2 halfSize   = inRectPosition.zw / 2.0;
    vec2 localPos   = inPosition - rectCenter;

    float alpha = sdf_rrect(localPos, halfSize, inRadius, 0.0, inFactor);

    outColor = inColor * alpha;
}
