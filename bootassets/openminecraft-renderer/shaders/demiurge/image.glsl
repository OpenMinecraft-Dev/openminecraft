#ifdef VERTEX_SHADER
#include "basics/geometry.glsl"
#include "basics/rotation.glsl"
#include "basics/vertexgen.glsl"

#vertex

layout(location = 0) out vec4 imageColor;
layout(location = 1) out vec2 imagePosition;
layout(location = 2) out vec4 imageRadius;
layout(location = 3) out vec4 imageRectPosition;
layout(location = 4) out float imageFactor;
layout(location = 5) out vec2 imageUv;
layout(location = 6) flat out float imageFillType;

uniform ScreenData
{
    float width;
    float height;
}
ubo;

void main()
{
    vec2 inPosition = vertexgen_quad_normal();
    vec2 screenPos = inRectPos.xy - vec2(10) + inPosition.xy * (inRectPos.zw + vec2(20));
    vec2 rectCenter = geom_rectCenterPos(inRectPos);
    vec3 localPos = vec3(screenPos - rectCenter, 0.0);

    gl_Position = vec4(geom_toNdc(rectCenter + rotation_quat(inRectRotation, localPos).xy, ubo.width, ubo.height),
                       inRectDepth, 1.0);
    imageColor = inRectColor;
    imagePosition = screenPos;
    imageRadius = inRectRadius;
    imageRectPosition = inRectPos;
    imageFactor = inRectFactor;
    imageUv = inPosition;
    imageFillType = inFillType;
}
#endif

#ifdef FRAGMENT_SHADER
#include "basics/sdf/sdf_rrect.glsl"

layout(location = 0) in vec4 imageColor;
layout(location = 1) in vec2 imagePosition;
layout(location = 2) in vec4 imageRadius;
layout(location = 3) in vec4 imageRectPosition;
layout(location = 4) in float imageFactor;
layout(location = 5) in vec2 imageUv;
layout(location = 6) flat in float imageFillType;

layout(location = 0) out vec4 outColor;

uniform ScreenData
{
    float width;
    float height;
}
ubo;
uniform sampler2D inTexture;

#define FILLTYPE_FIT 0
#define FILLTYPE_CONTAIN 1
#define FILLTYPE_COVER 2

void main()
{
    float dummy = ubo.width + ubo.height;
    vec2 rectCenter = geom_rectCenterPos(imageRectPosition);
    vec2 halfSize = imageRectPosition.zw / 2.0;
    vec2 localPos = imagePosition - rectCenter;

    float alpha = sdf_rrect(localPos, halfSize, imageRadius, 0.0, imageFactor);

    vec2 imageSize = vec2(textureSize(inTexture, 0));
    vec2 rectSize = imageRectPosition.zw;
    vec2 uv;

    if (int(imageFillType) == FILLTYPE_FIT)
    {
        uv = imageUv;
    }
    else
    {
        float scale = int(imageFillType) == FILLTYPE_CONTAIN ? min(rectSize.x / imageSize.x, rectSize.y / imageSize.y)
                                                             : max(rectSize.x / imageSize.x, rectSize.y / imageSize.y);
        vec2 scaledImgSize = imageSize * scale;
        uv = (imagePosition - rectCenter) / scaledImgSize + 0.5;
        if (int(imageFillType) == FILLTYPE_CONTAIN && (uv.x < 0.0 || uv.x > 1.0 || uv.y < 0.0 || uv.y > 1.0))
        {
            alpha = 0.0;
        }
    }

    outColor = imageColor * alpha * texture(inTexture, uv);
}
#endif