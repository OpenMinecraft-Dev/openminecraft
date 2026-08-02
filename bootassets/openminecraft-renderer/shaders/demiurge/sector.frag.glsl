#version 450 core
#extension GL_ARB_separate_shader_objects : enable

#include "basics/sdf/sdf_rrect.glsl"

layout(location = 0) in vec4 inColor;
layout(location = 1) in vec2 inPosition;
layout(location = 2) in vec4 inSectorPosition;
layout(location = 3) in float inSectorRadius;

layout(location = 0) out vec4 outColor;

void main() {
    vec2 rectCenter = geom_rectCenterPos(inSectorPosition);
    vec2 halfSize   = inSectorPosition.zw / 2.0;
    float weight = 1 - distance(rectCenter, inPosition) / min(halfSize.x, halfSize.y);
    outColor = inColor * smoothstep(-0.04, 0, weight);
}
