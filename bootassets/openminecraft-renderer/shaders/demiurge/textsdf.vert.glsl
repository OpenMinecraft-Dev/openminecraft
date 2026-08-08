#version 330 core
#extension GL_ARB_separate_shader_objects : enable

#include "basics/geometry.glsl"
#include "basics/texelbuf.glsl"

layout(location = 0) in vec2 inPosition;
layout(location = 1) in vec4 inTextPos;
layout(location = 2) in vec4 inTextColor;
layout(location = 3) in float inTextDepth;
layout(location = 4) in float inTextFactor;
layout(location = 5) in int inTextGlyphId;

layout(location = 0) out vec4 textColor;
layout(location = 1) out vec2 textUv;
layout(location = 2) flat out int textGlyphId;
layout(location = 3) out float textFactor;

uniform ScreenData {
    float width;
    float height;
} ubo;
uniform samplerBuffer GlyphData;

vec4 getBBox() {

    return vec4(texelFetchF(GlyphData, 0 + inTextGlyphId), texelFetchF(GlyphData, 1 + inTextGlyphId), texelFetchF(GlyphData, 2 + inTextGlyphId), texelFetchF(GlyphData, 3 + inTextGlyphId));
}

void main() {
    vec2 screenPos = inTextPos.xy + inPosition.xy * inTextPos.zw;

    gl_Position = vec4(geom_toNdc(screenPos, ubo.width, ubo.height), inTextDepth, 1.0);
    textColor = inTextColor;
    vec4 bb = getBBox();
    textUv = bb.xz + vec2(inPosition.x, 1 - inPosition.y) * (bb.yw - bb.xz);
    textGlyphId = inTextGlyphId;
    textFactor = inTextFactor;
}
