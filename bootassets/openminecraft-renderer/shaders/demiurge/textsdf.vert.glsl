#version 450 core
#extension GL_ARB_separate_shader_objects : enable

#include "basics/geometry.glsl"

layout(location = 0) in vec2 inPosition;
layout(location = 1) in vec4 inTextPos;
layout(location = 2) in vec4 inTextColor;
layout(location = 3) in float inTextDepth;
layout(location = 4) in float inTextHeight;
layout(location = 5) in float inTextFactor;
layout(location = 6) in int inTextGlyphId;

layout(location = 0) out vec4 outColor;
layout(location = 1) out vec2 outTextUv;
layout(location = 2) flat out int outTextGlyphId;
layout(location = 3) out float outTextFactor;

layout(binding = 0) uniform UniformBufferObject {
    float width;
    float height;
} ubo;

layout(std430, binding = 1) readonly buffer GlyphData {
    float data[];
} glyphBuffer;

vec4 getBBox() {
    return vec4(glyphBuffer.data[0 + inTextGlyphId], glyphBuffer.data[1 + inTextGlyphId],
                glyphBuffer.data[2 + inTextGlyphId], glyphBuffer.data[3 + inTextGlyphId]);
}

void main() {
    vec2 screenPos = inTextPos.xy + inPosition.xy * inTextPos.zw;

    gl_Position = vec4(geom_toNdc(screenPos, ubo.width, ubo.height), inTextDepth, 1.0);
#ifdef VULKAN
    gl_Position.y = -gl_Position.y;
#endif
    outColor = inTextColor;
    vec4 bb = getBBox();
    outTextUv = bb.xz + vec2(inPosition.x, 1 - inPosition.y) * (bb.yw - bb.xz);
    outTextGlyphId = inTextGlyphId;
    outTextFactor = inTextFactor;
}
