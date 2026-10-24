#ifndef OM_DEMIURGE_ELEMENT_SHADERSCENE_HPP
#define OM_DEMIURGE_ELEMENT_SHADERSCENE_HPP

#include "glm/ext/vector_float4.hpp"
#include "openminecraft/renderer/common/demiurge/element/om_demiurge_element_channel.hpp"
#include "glm/gtc/quaternion.hpp"

namespace openminecraft::renderer::common::demiurge::element
{
struct OMDemiurgeElementShaderScene
{
    glm::vec4 position;
    float depth;
};

template <> inline auto objectGetDepth<OMDemiurgeElementShaderScene>(OMDemiurgeElementShaderScene &obj) -> float
{
    return obj.depth;
}
template <>
inline auto objectSetDepth<OMDemiurgeElementShaderScene>(OMDemiurgeElementShaderScene &obj, float depth) -> void
{
    obj.depth = depth;
}
} // namespace openminecraft::renderer::common::demiurge::element

#endif
