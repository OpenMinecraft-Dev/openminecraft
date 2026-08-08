#version 330 core
#extension GL_ARB_separate_shader_objects : enable

#include "basics/sdf/sdf_sector.glsl"

layout(location = 0) in vec4 secColor;
layout(location = 1) in vec2 secPosition;
layout(location = 2) in vec4 secSectorPosition;
layout(location = 3) in vec2 secSectorAngle;
layout(location = 4) in float secSectorFactor;

layout(location = 0) out vec4 outColor;

void main() {
    vec2 sectorCenter = geom_rectCenterPos(secSectorPosition);
    vec2 halfSize   = secSectorPosition.zw / 2.0;

    outColor = secColor * sdf_sector(secPosition, halfSize, sectorCenter, secSectorAngle, 0, secSectorFactor);
}
