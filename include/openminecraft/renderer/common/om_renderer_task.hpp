#ifndef OM_RENDERER_TASK
#define OM_RENDERER_TASK

#include "openminecraft/renderer/common/om_renderer_buffer.hpp"
#include "openminecraft/renderer/common/om_renderer_pipeline.hpp"
#include "openminecraft/renderer/common/om_renderer_rendertarget.hpp"
#include "openminecraft/renderer/om_renderer_object.hpp"
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
    virtual void bindVertexBuffer(std::vector<OMRendererBuffer *> buffer) = 0;
    virtual void bindIndexBuffer(OMRendererBuffer *buffer) = 0;
    virtual void bindTarget(OMRendererRenderTarget *target) = 0;
    virtual void draw(uint64_t vertexCount) = 0;
    virtual void finish() = 0;

    inline auto objType() -> OMRendererObjectType override
    {
        return Task;
    }

    inline auto dependOn(OMRendererTask *task)
    {
        dependTasks.push_back(task);
        task->relied = true;
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

    bool relied = false;

  private:
    std::vector<OMRendererTask *> dependTasks;
};
} // namespace openminecraft::renderer::common

#endif
