#version 410 core
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

layout(location = 0) in vec4 inColor;
layout(location = 1) in vec2 inPosition;
layout(location = 2) in vec4 inRadius;
layout(location = 3) in vec4 inRectPosition;

layout(location = 0) out vec4 outColor;

float rrectSdf(vec2 p, vec2 halfSize, float radius, float t, float sme) {
    vec2 q = abs(p) - halfSize + radius;
    float d = length(max(q, 0.0)) + min(max(q.x, q.y), 0.0) - radius;
    return 1.0 - smoothstep(t, t + sme, d);
}

void main() {
    vec2 rectCenter = inRectPosition.xy + inRectPosition.zw / 2.0;
    vec2 halfSize   = inRectPosition.zw / 2.0;
    vec2 localPos   = inPosition - rectCenter;

    float alpha = rrectSdf(localPos, halfSize, 30.0, 0.0, 2.0);

    outColor = inColor * alpha;
}
