#ifndef OM_DEMIURGE_ELEMENT_RECT_HPP
#define OM_DEMIURGE_ELEMENT_RECT_HPP

#include "glm/ext/vector_float4.hpp"
namespace openminecraft::renderer::common::demiurge::element
{
struct OMDemiurgeElementRect
{
    glm::vec4 position;
    glm::vec4 color;
    float depth;
};
} // namespace openminecraft::renderer::common::demiurge::element

#endif
