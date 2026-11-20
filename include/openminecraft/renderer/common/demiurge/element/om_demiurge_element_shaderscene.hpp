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
    uint32_t depth;
    glm::vec2 scale = glm::vec2(1.0f);
};

template <> inline auto objectGetDepth<OMDemiurgeElementShaderScene>(OMDemiurgeElementShaderScene &obj) -> uint32_t
{
    return obj.depth;
}
template <>
inline auto objectSetDepth<OMDemiurgeElementShaderScene>(OMDemiurgeElementShaderScene &obj, uint32_t depth) -> void
{
    obj.depth = depth;
}
} // namespace openminecraft::renderer::common::demiurge::element

#endif
