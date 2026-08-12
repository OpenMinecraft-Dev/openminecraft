#version 330 core
#extension GL_ARB_separate_shader_objects : enable

#include "basics/sdf/sdf_rrect.glsl"

layout(location = 0) in vec4 rrectColor;
layout(location = 1) in vec2 rrectPosition;
layout(location = 2) in vec4 rrectRadius;
layout(location = 3) in vec4 rrectRectPosition;
layout(location = 4) in float rrectFactor;

layout(location = 0) out vec4 outColor;

void main()
{
    vec2 rectCenter = geom_rectCenterPos(rrectRectPosition);
    vec2 halfSize = rrectRectPosition.zw / 2.0;
    vec2 localPos = rrectPosition - rectCenter;

    float alpha = sdf_rrect(localPos, halfSize, rrectRadius, 0.0, rrectFactor);

    outColor = rrectColor * alpha;
}
