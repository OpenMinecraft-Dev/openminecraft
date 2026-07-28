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
    float depth;
};

template <> inline auto objectGetDepth<OMDemiurgeElementRect>(OMDemiurgeElementRect &obj) -> float
{
    return obj.depth;
}
template <> inline auto objectSetDepth<OMDemiurgeElementRect>(OMDemiurgeElementRect &obj, float depth) -> void
{
    obj.depth = depth;
}
} // namespace openminecraft::renderer::common::demiurge::element

#endif
