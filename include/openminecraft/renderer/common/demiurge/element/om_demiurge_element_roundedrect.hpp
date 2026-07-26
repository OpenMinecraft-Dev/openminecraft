#ifndef OM_DEMIURGE_ELEMENT_ROUNDEDRECT_HPP
#define OM_DEMIURGE_ELEMENT_ROUNDEDRECT_HPP

#include "glm/ext/vector_float4.hpp"
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
} // namespace openminecraft::renderer::common::demiurge::element

#endif
