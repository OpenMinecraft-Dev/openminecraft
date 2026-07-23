#ifndef OM_DEMIURGE_ELEMENT_RECT_HPP
#define OM_DEMIURGE_ELEMENT_RECT_HPP

#include "glm/ext/vector_float4.hpp"
namespace openminecraft::renderer::common::demiurge::element
{
struct OMDemiurgeElementRect
{
    float depth;
    glm::vec4 position;
    glm::vec4 color;
};
} // namespace openminecraft::renderer::common::demiurge::element

#endif
