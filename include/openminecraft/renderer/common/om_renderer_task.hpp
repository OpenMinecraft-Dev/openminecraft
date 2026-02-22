#ifndef OM_RENDERER_TASK
#define OM_RENDERER_TASK

#include "openminecraft/renderer/common/om_renderer_buffer.hpp"
#include "openminecraft/renderer/common/om_renderer_pipeline.hpp"
#include "openminecraft/renderer/common/om_renderer_rendertarget.hpp"
#include "openminecraft/renderer/om_renderer_object.hpp"
namespace openminecraft::renderer
{
class OMRenderer;
}

namespace openminecraft::renderer::common
{
class OMRendererTask : public OMRendererObject
{
  public:
    OMRendererTask(OMRenderer *renderer)
    {
    }

    virtual ~OMRendererTask()
    {
    }

    virtual void bindPipeline(OMRendererPipeline *pipeline) = 0;
    virtual void bindVertexBuffer(std::vector<OMRendererBuffer *> buffer) = 0;
    virtual void bindIndexBuffer(OMRendererBuffer *buffer) = 0;
    virtual void bindTarget(OMRendererRenderTarget *target) = 0;
    virtual void draw(uint64_t vertexCount) = 0;
    virtual void finish() = 0;

    OMRendererObjectType objType() override
    {
        return Task;
    }
};
} // namespace openminecraft::renderer::common

#endif
