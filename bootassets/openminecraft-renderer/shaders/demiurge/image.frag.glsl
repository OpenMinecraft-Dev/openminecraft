#version 330 core
#extension GL_ARB_separate_shader_objects : enable

#include "basics/sdf/sdf_rrect.glsl"

layout(location = 0) in vec4 inColor;
layout(location = 1) in vec2 inPosition;
layout(location = 2) in vec4 inRadius;
layout(location = 3) in vec4 inRectPosition;
layout(location = 4) in float inFactor;
layout(location = 5) in vec2 inUv;
layout(location = 6) flat in float inFillType;

layout(location = 0) out vec4 outColor;

uniform ScreenData {
    float width;
    float height;
} ubo;
uniform sampler2D inTexture;

#define FILLTYPE_FIT 0
#define FILLTYPE_CONTAIN 1
#define FILLTYPE_COVER 2

void main() {
    vec2 rectCenter = geom_rectCenterPos(inRectPosition);
    vec2 halfSize   = inRectPosition.zw / 2.0;
    vec2 localPos   = inPosition - rectCenter;

    float alpha = sdf_rrect(localPos, halfSize, inRadius, 0.0, inFactor);

    vec2 imageSize = vec2(textureSize(inTexture, 0));
    vec2 rectSize  = inRectPosition.zw;
    vec2 uv;

    if (int(inFillType) == FILLTYPE_FIT) {
        uv = inUv;
    }
    else {
        float scale = int(inFillType) == FILLTYPE_CONTAIN ? min(rectSize.x / imageSize.x, rectSize.y / imageSize.y)
                                       : max(rectSize.x / imageSize.x, rectSize.y / imageSize.y);
        vec2 scaledImgSize = imageSize * scale;
        uv = (inPosition - rectCenter) / scaledImgSize + 0.5;
        if (int(inFillType) == FILLTYPE_CONTAIN && (uv.x < 0.0 || uv.x > 1.0 || uv.y < 0.0 || uv.y > 1.0)) {
            alpha = 0.0;
        }
    }

    outColor = inColor * alpha * texture(inTexture, uv);
}
