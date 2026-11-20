#ifndef OM_DEMIURGE_ELEMENT_RECT_HPP
#define OM_DEMIURGE_ELEMENT_RECT_HPP

#include "glm/ext/vector_float4.hpp"
#include "openminecraft/renderer/common/demiurge/element/om_demiurge_element_channel.hpp"
namespace openminecraft::renderer::common::demiurge::element
{
struct OMDemiurgeElementRect
{
    glm::vec4 position;
    glm::vec4 color;
    uint32_t depth;
    glm::vec2 scale = glm::vec2(1.0f);
};

template <> inline auto objectGetDepth<OMDemiurgeElementRect>(OMDemiurgeElementRect &obj) -> uint32_t
{
    return obj.depth;
}
template <> inline auto objectSetDepth<OMDemiurgeElementRect>(OMDemiurgeElementRect &obj, uint32_t depth) -> void
{
    obj.depth = depth;
}
} // namespace openminecraft::renderer::common::demiurge::element

#endif
