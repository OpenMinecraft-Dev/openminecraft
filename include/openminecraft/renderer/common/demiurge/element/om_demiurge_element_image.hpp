#ifndef OM_DEMIURGE_ELEMENT_IMAGE_HPP
#define OM_DEMIURGE_ELEMENT_IMAGE_HPP

#include "glm/ext/vector_float4.hpp"
#include "openminecraft/renderer/common/demiurge/element/om_demiurge_element_channel.hpp"
#include "glm/gtc/quaternion.hpp"

namespace openminecraft::renderer::common::demiurge::element
{
struct OMDemiurgeElementImage
{
    glm::vec4 position;
    glm::vec4 color;
    glm::vec4 radius;
    glm::quat rotation;
    float factor;
    uint32_t depth;
    float fillMode;
    glm::vec2 scale = glm::vec2(1.0f);
};

template <> inline auto objectGetDepth<OMDemiurgeElementImage>(OMDemiurgeElementImage &obj) -> uint32_t
{
    return obj.depth;
}
template <> inline auto objectSetDepth<OMDemiurgeElementImage>(OMDemiurgeElementImage &obj, uint32_t depth) -> void
{
    obj.depth = depth;
}
} // namespace openminecraft::renderer::common::demiurge::element

#endif
