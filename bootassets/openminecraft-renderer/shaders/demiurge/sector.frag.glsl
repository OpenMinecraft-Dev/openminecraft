#version 330 core
#extension GL_ARB_separate_shader_objects : enable

#include "basics/sdf/sdf_sector.glsl"

layout(location = 0) in vec4 inColor;
layout(location = 1) in vec2 inPosition;
layout(location = 2) in vec4 inSectorPosition;
layout(location = 3) in vec2 inSectorAngle;
layout(location = 4) in float inSectorFactor;

layout(location = 0) out vec4 outColor;

void main() {
    vec2 sectorCenter = geom_rectCenterPos(inSectorPosition);
    vec2 halfSize   = inSectorPosition.zw / 2.0;

    outColor = inColor * sdf_sector(inPosition, halfSize, sectorCenter, inSectorAngle, 0, inSectorFactor);
}
