#ifndef OM_DEMIURGE_ELEMENT_ROUNDEDRECT_HPP
#define OM_DEMIURGE_ELEMENT_ROUNDEDRECT_HPP

#include "glm/ext/vector_float4.hpp"
#include "openminecraft/renderer/common/demiurge/element/om_demiurge_element_channel.hpp"
namespace openminecraft::renderer::common::demiurge::element
{
struct OMDemiurgeElementRoundedRect
{
    glm::vec4 position;
    glm::vec4 color;
    glm::vec4 radius;
    float factor;
    float depth;
};

template <> inline auto objectGetDepth<OMDemiurgeElementRoundedRect>(OMDemiurgeElementRoundedRect &obj) -> float
{
    return obj.depth;
}
template <>
inline auto objectSetDepth<OMDemiurgeElementRoundedRect>(OMDemiurgeElementRoundedRect &obj, float depth) -> void
{
    obj.depth = depth;
}
} // namespace openminecraft::renderer::common::demiurge::element

#endif
