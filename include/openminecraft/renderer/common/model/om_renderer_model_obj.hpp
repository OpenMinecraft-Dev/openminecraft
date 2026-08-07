#ifndef OM_RENDERER_MODEL_OBJ_HPP
#define OM_RENDERER_MODEL_OBJ_HPP

#include "glm/ext/vector_float2.hpp"
#include "glm/ext/vector_float3.hpp"
#include "openminecraft/renderer/common/basics/om_vertex_format.hpp"
#include "openminecraft/renderer/common/om_renderer_buffer.hpp"
#include "openminecraft/renderer/om_renderer_layer.hpp"
#include <istream>
namespace openminecraft::renderer::common::model
{
class OMRendererModelObj
{
    using Vertex = basics::OMVertex<glm::vec3, glm::vec2, glm::vec3>;

  public:
    OMRendererModelObj(OMRenderer *, std::istream *);
    ~OMRendererModelObj();

    basics::OMVertexFormat format;
    OMRendererBuffer *vertexData, *vertexIndex;
    uint32_t vertexCount;
};
} // namespace openminecraft::renderer::common::model

#endif
