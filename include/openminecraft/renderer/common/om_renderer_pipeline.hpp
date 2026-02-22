#ifndef OM_RENDERER_PIPELINE_HPP
#define OM_RENDERER_PIPELINE_HPP

#include "openminecraft/renderer/common/basics/om_vertex_format.hpp"
#include "openminecraft/renderer/common/om_renderer_buffer.hpp"
#include "openminecraft/renderer/common/om_renderer_rendertarget.hpp"
#include "openminecraft/renderer/common/om_renderer_texture.hpp"
#include "openminecraft/renderer/om_renderer_object.hpp"
#include <memory>
namespace openminecraft::renderer
{
class OMRenderer;
}

namespace openminecraft::renderer::common
{
class OMShader;
class OMRendererRenderTarget;
enum OMRendererPipelineInputType
{
    ImageSampler,
    UniformBuffer
};
class OMRendererPipeline : public OMRendererObject
{
  public:
    OMRendererPipeline(OMRenderer *renderer)
    {
    }
    virtual ~OMRendererPipeline()
    {
    }

    virtual void appendInput(OMRendererPipelineInputType) = 0;
    virtual void attachShader(std::shared_ptr<OMShader> shader) = 0;
    virtual void vertexFormat(basics::OMVertexFormat format) = 0;
    virtual void bindOutput(OMRendererRenderTarget *target) = 0;
    virtual void build() = 0;

    virtual void bindInput(int idx, common::OMRendererBuffer *buff) = 0;
    virtual void bindInput(int idx, common::OMRendererTexture *texture) = 0;

    OMRendererObjectType objType() override
    {
        return Pipeline;
    }
};
} // namespace openminecraft::renderer::common

#endif
