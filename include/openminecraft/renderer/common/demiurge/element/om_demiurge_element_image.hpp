#ifndef OM_DEMIURGE_ELEMENT_IMAGE_HPP
#define OM_DEMIURGE_ELEMENT_IMAGE_HPP

#include "glm/ext/vector_float4.hpp"
#include "openminecraft/renderer/common/demiurge/element/om_demiurge_element_channel.hpp"
namespace openminecraft::renderer::common::demiurge::element
{
struct OMDemiurgeElementImage
{
    glm::vec4 position;
    glm::vec4 color;
    glm::vec4 radius;
    float factor;
    float depth;
    float fillMode;
};

template <> inline auto objectGetDepth<OMDemiurgeElementImage>(OMDemiurgeElementImage &obj) -> float
{
    return obj.depth;
}
template <> inline auto objectSetDepth<OMDemiurgeElementImage>(OMDemiurgeElementImage &obj, float depth) -> void
{
    obj.depth = depth;
}
} // namespace openminecraft::renderer::common::demiurge::element

#endif
