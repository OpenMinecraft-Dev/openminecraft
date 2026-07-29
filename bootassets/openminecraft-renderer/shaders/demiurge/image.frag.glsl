#version 450 core
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

layout(location = 0) in vec4 inColor;
layout(location = 1) in vec2 inPosition;
layout(location = 2) in vec4 inRadius;
layout(location = 3) in vec4 inRectPosition;
layout(location = 4) in float inFactor;
layout(location = 5) in vec2 inUv;
layout(location = 6) flat in float inFillType;

layout(location = 0) out vec4 outColor;

layout(binding = 1) uniform sampler2D inTexture;

int selectQuadrant(vec2 p) {
    ivec2 mask = ivec2(step(0.0, p));
    return mask.y << 1 | ((1 - mask.x) ^ mask.y);
}

float rrectSdf(vec2 p, vec2 halfSize, float radius, float thickness, float smoothEdge) {
    radius = inRadius[selectQuadrant(p)];
    radius = min(radius, min(halfSize.x, halfSize.y));
    vec2 centerDis = abs(p) - halfSize + radius;
    float edgeDis = length(max(centerDis, 0.0)) + min(max(centerDis.x, centerDis.y), 0.0) - radius;
    return 1.0 - smoothstep(thickness, thickness + smoothEdge, edgeDis);
}

void main() {
    vec2 rectCenter = inRectPosition.xy + inRectPosition.zw / 2.0;
    vec2 halfSize   = inRectPosition.zw / 2.0;
    vec2 localPos   = inPosition - rectCenter;

    float alpha = rrectSdf(localPos, halfSize, inRadius.x, 0.0, inFactor);

    vec2 imageSize = vec2(textureSize(inTexture, 0));
    vec2 rectSize  = inRectPosition.zw;
    vec2 uv;

    if (int(inFillType) == 0) {
        uv = inUv;
    }
    else {
        float scale = int(inFillType) == 1 ? min(rectSize.x / imageSize.x, rectSize.y / imageSize.y)
                                       : max(rectSize.x / imageSize.x, rectSize.y / imageSize.y);
        vec2 scaledImgSize = imageSize * scale;
        uv = (inPosition - inRectPosition.xy - inRectPosition.zw / 2.0) / scaledImgSize + 0.5;
        if (int(inFillType) == 1 && (uv.x < 0.0 || uv.x > 1.0 || uv.y < 0.0 || uv.y > 1.0))
            alpha = 0.0;
    }

    outColor = inColor * alpha * texture(inTexture, uv);
}
