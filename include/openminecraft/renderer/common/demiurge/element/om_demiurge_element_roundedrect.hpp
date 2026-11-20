#ifndef OM_DEMIURGE_ELEMENT_ROUNDEDRECT_HPP
#define OM_DEMIURGE_ELEMENT_ROUNDEDRECT_HPP

#include "glm/ext/vector_float2.hpp"
#include "glm/ext/vector_float4.hpp"
#include "glm/gtc/quaternion.hpp"
#include "openminecraft/renderer/common/demiurge/element/om_demiurge_element_channel.hpp"
namespace openminecraft::renderer::common::demiurge::element
{
struct OMDemiurgeElementRoundedRect
{
    glm::vec4 position;
    glm::vec4 color;
    glm::vec4 radius;
    glm::quat rotation;
    float factor;
    uint32_t depth;
    // Per-axis scale around the rect center (applied in the shader, so it is not
    // affected by layout pixel snapping).
    glm::vec2 scale = glm::vec2(1.0f);
    // 屏幕像素空间的裁剪矩形（x, y, w, h）；w/h <= 0 表示不裁剪
    glm::vec4 clipPosition = glm::vec4(0.0f);
    float clipRadius = 0.0f;
};

template <> inline auto objectGetDepth<OMDemiurgeElementRoundedRect>(OMDemiurgeElementRoundedRect &obj) -> uint32_t
{
    return obj.depth;
}
template <>
inline auto objectSetDepth<OMDemiurgeElementRoundedRect>(OMDemiurgeElementRoundedRect &obj, uint32_t depth) -> void
{
    obj.depth = depth;
}
} // namespace openminecraft::renderer::common::demiurge::element

#endif
