#ifndef OM_RENDERER_TASK
#define OM_RENDERER_TASK

#include "openminecraft/renderer/common/om_renderer_buffer.hpp"
#include "openminecraft/renderer/common/om_renderer_pipeline.hpp"
#include "openminecraft/renderer/common/om_renderer_rendertarget.hpp"
#include "openminecraft/renderer/om_renderer_object.hpp"
#include <cstdint>
#include <vector>
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

    virtual ~OMRendererTask() = default;

    virtual void bindPipeline(OMRendererPipeline *pipeline) = 0;
    inline auto pipeline(OMRendererPipeline *pipeline) -> OMRendererTask *
    {
        bindPipeline(pipeline);
        return this;
    }
    virtual void bindVertexBuffer(std::vector<OMRendererBuffer *> buffer) = 0;
    inline auto vertexBuffer(std::vector<OMRendererBuffer *> buffer) -> OMRendererTask *
    {
        bindVertexBuffer(buffer);
        return this;
    }
    virtual void bindIndexBuffer(OMRendererBuffer *buffer) = 0;
    inline auto indexBuffer(OMRendererBuffer *buffer) -> OMRendererTask *
    {
        bindIndexBuffer(buffer);
        return this;
    }
    virtual void bindTarget(OMRendererRenderTarget *target) = 0;
    inline auto target(OMRendererRenderTarget *target) -> OMRendererTask *
    {
        bindTarget(target);
        return this;
    }
    virtual void draw(uint64_t vertexCount) = 0;
    inline auto drawN(uint64_t vertexCount) -> OMRendererTask *
    {
        draw(vertexCount);
        return this;
    }
    virtual void drawInstance(uint64_t vertexCount, uint64_t instanceCount) = 0;
    inline auto drawInstanceN(uint64_t vertexCount, uint64_t instanceCount) -> OMRendererTask *
    {
        drawInstance(vertexCount, instanceCount);
        return this;
    }
    virtual void finish() = 0;
    inline auto finishN() -> OMRendererTask *
    {
        finish();
        return this;
    }
    virtual void clear() = 0;
    inline auto clearN() -> OMRendererTask *
    {
        clear();
        return this;
    }

    inline auto objType() -> OMRendererObjectType override
    {
        return Task;
    }

    inline auto dependOn(OMRendererTask *task) -> OMRendererTask *
    {
        dependTasks.push_back(task);
        return this;
    }

    inline auto isTopTask() -> bool
    {
        return dependTasks.empty();
    }

    inline auto executable() -> bool
    {
        for (auto t : dependTasks)
        {
            if (!t->solved)
            {
                return false;
            }
        }
        return true;
    }
    bool solved = false;

  private:
    std::vector<OMRendererTask *> dependTasks;
};
} // namespace openminecraft::renderer::common

#endif
