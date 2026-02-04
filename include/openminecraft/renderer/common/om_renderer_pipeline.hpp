#ifndef OM_RENDERER_PIPELINE_HPP
#define OM_RENDERER_PIPELINE_HPP

#include "openminecraft/renderer/common/basics/om_vertex_format.hpp"
#include <memory>
namespace openminecraft::renderer
{
class OMRenderer;
}

namespace openminecraft::renderer::common
{
class OMShader;
class OMRendererPipeline
{
  public:
    OMRendererPipeline(OMRenderer *renderer)
    {
    }
    virtual ~OMRendererPipeline() = 0;

    virtual void attachShader(std::shared_ptr<OMShader> shader) = 0;
    virtual void vertexFormat(basics::OMVertexFormat format) = 0;
};
} // namespace openminecraft::renderer::common

#endif
