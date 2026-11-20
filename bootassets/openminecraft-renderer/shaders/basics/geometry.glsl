#ifndef GEOMETRY_GLSL
#define GEOMETRY_GLSL

int geom_selectQuadrant(vec2 p)
{
    ivec2 mask = ivec2(step(0.0, p));
    return mask.y << 1 | ((1 - mask.x) ^ mask.y);
}

vec2 geom_rectCenterPos(vec4 pos)
{
    return pos.xy + pos.zw / 2;
}

vec2 geom_toNdc(vec2 screenPos, float width, float height)
{
    float ndcX = (screenPos.x / width) * 2.0 - 1.0;
    float ndcY = 1.0 - (screenPos.y / height) * 2.0;

    return vec2(ndcX, ndcY);
}

// The UI layer packs a 15-bit z-order in bits [30:16]. Larger z-order means
// closer to the viewer, so it maps to a smaller clip-space depth. Keep the
// bottom-most z-order just inside the cleared depth (1.0); the UI pipelines use
// LessOrEqual so equal-z elements still blend in submit order.
float geom_zOrderDepth(uint layer)
{
    uint z = (layer >> 16u) & 0x7FFFu;
    return 1.0 - float(z) / 32767.0;
}

#endif
