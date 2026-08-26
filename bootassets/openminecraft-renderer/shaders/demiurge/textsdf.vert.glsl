#version 330 core
#extension GL_ARB_separate_shader_objects : enable

#include "basics/geometry.glsl"
#include "basics/texelbuf.glsl"
#include "basics/vertexgen.glsl"

#vertex

layout(location = 0) out vec4 textColor;
layout(location = 1) out vec2 textUv;
layout(location = 2) flat out int textGlyphId;
layout(location = 3) out float textFactor;

uniform ScreenData
{
    float width;
    float height;
}
ubo;
uniform samplerBuffer GlyphData;

vec4 getBBox()
{
    return vec4(texelFetchF(GlyphData, 0 + inTextGlyphId), texelFetchF(GlyphData, 1 + inTextGlyphId),
                texelFetchF(GlyphData, 2 + inTextGlyphId), texelFetchF(GlyphData, 3 + inTextGlyphId));
}

void main()
{
    vec2 inPosition = (vertexgen_quad_normal() - vec2(0.5)) * 3 + vec2(0.5);
    vec2 screenPos = inTextPos.xy + inPosition.xy * inTextPos.zw;

    gl_Position = vec4(geom_toNdc(screenPos, ubo.width, ubo.height), inTextDepth, 1.0);
    textColor = inTextColor;
    vec4 bb = getBBox();
    textUv = bb.xz + vec2(inPosition.x, 1 - inPosition.y) * (bb.yw - bb.xz);
    textGlyphId = inTextGlyphId;
    textFactor = inTextFactor;
}
