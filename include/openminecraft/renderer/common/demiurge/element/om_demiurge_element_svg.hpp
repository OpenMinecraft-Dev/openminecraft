#ifndef OM_DEMIURGE_ELEMENT_SVG_HPP
#define OM_DEMIURGE_ELEMENT_SVG_HPP

#include "glm/ext/vector_float4.hpp"
#include "glm/gtc/quaternion.hpp"
#include "openminecraft/renderer/common/demiurge/element/om_demiurge_element_channel.hpp"
namespace openminecraft::renderer::common::demiurge::element
{
struct OMDemiurgeElementSvg
{
    glm::vec4 position;
    glm::vec4 color;
    glm::quat rotation;
    uint32_t depth;
    float factor;
    uint32_t glyphId;
    glm::vec2 scale = glm::vec2(1.0f);
};

template <> inline auto objectGetDepth<OMDemiurgeElementSvg>(OMDemiurgeElementSvg &obj) -> uint32_t
{
    return obj.depth;
}
template <> inline auto objectSetDepth<OMDemiurgeElementSvg>(OMDemiurgeElementSvg &obj, uint32_t depth) -> void
{
    obj.depth = depth;
}
} // namespace openminecraft::renderer::common::demiurge::element

#endif