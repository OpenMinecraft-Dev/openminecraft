#ifndef OM_DEMIURGE_ELEMENT_TEXTSDF
#define OM_DEMIURGE_ELEMENT_TEXTSDF

#include "glm/ext/vector_float4.hpp"
#include "openminecraft/renderer/common/demiurge/element/om_demiurge_element_channel.hpp"
namespace openminecraft::renderer::common::demiurge::element
{
struct OMDemiurgeElementTextSdf
{
    glm::vec4 position;
    glm::vec4 color;
    float depth;
    float factor;
    uint32_t glyphIndex;
};

template <> inline auto objectGetDepth<OMDemiurgeElementTextSdf>(OMDemiurgeElementTextSdf &obj) -> float
{
    return obj.depth;
}
template <> inline auto objectSetDepth<OMDemiurgeElementTextSdf>(OMDemiurgeElementTextSdf &obj, float depth) -> void
{
    obj.depth = depth;
}
} // namespace openminecraft::renderer::common::demiurge::element

#endif
