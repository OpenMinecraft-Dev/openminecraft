#ifndef OM_DEMIURGE_ELEMENT_SECTOR_HPP
#define OM_DEMIURGE_ELEMENT_SECTOR_HPP

#include "glm/ext/vector_float4.hpp"
#include "openminecraft/renderer/common/demiurge/element/om_demiurge_element_channel.hpp"
namespace openminecraft::renderer::common::demiurge::element
{
struct OMDemiurgeElementSector
{
    glm::vec4 position;
    glm::vec4 color;
    float radius;
    float beginAngle;
    float endAngle;
    float factor;
    float depth;
};

template <> inline auto objectGetDepth<OMDemiurgeElementSector>(OMDemiurgeElementSector &obj) -> float
{
    return obj.depth;
}
template <> inline auto objectSetDepth<OMDemiurgeElementSector>(OMDemiurgeElementSector &obj, float depth) -> void
{
    obj.depth = depth;
}
} // namespace openminecraft::renderer::common::demiurge::element

#endif
