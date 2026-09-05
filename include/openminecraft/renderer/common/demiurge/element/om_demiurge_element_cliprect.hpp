#ifndef OM_DEMIURGE_ELEMENT_CLIPRECT_HPP
#define OM_DEMIURGE_ELEMENT_CLIPRECT_HPP

#include "glm/ext/vector_float4.hpp"
#include "openminecraft/renderer/common/demiurge/element/om_demiurge_element_channel.hpp"
namespace openminecraft::renderer::common::demiurge::element
{
struct OMDemiurgeElementClipRect
{
    glm::vec4 position;
    float depth;
};

template <> inline auto objectGetDepth<OMDemiurgeElementClipRect>(OMDemiurgeElementClipRect &obj) -> float
{
    return obj.depth;
}
template <> inline auto objectSetDepth<OMDemiurgeElementClipRect>(OMDemiurgeElementClipRect &obj, float depth) -> void
{
    obj.depth = depth;
}
} // namespace openminecraft::renderer::common::demiurge::element

#endif
