#ifndef OM_RENDERER_TASK
#define OM_RENDERER_TASK

#include "glm/ext/vector_float4.hpp"
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

    inline void setClearColor(glm::vec4 c)
    {
        colorClear = c;
    }
    inline auto clearColor(glm::vec4 color) -> OMRendererTask *
    {
        setClearColor(color);
        return this;
    }

    inline void setClearDepth(float d)
    {
        depthClear = d;
    }
    inline auto clearDepth(float depth) -> OMRendererTask *
    {
        setClearDepth(depth);
        return this;
    }

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
    virtual void bindVertexBufferInstanced(std::vector<common::OMRendererBuffer *> buffer, int off)
    {
        bindVertexBuffer(buffer);
    }
    inline auto vertexBufferInstanced(std::vector<common::OMRendererBuffer *> buffer, int off) -> OMRendererTask *
    {
        bindVertexBufferInstanced(buffer, off);
        return this;
    }
    virtual void bindIndexBuffer(OMRendererBuffer *buffer) = 0;
    inline auto indexBuffer(OMRendererBuffer *buffer) -> OMRendererTask *
    {
        bindIndexBuffer(buffer);
        return this;
    }
    virtual void bindIndirectBuffer(OMRendererBuffer *buffer) = 0;
    inline auto indirectBuffer(OMRendererBuffer *buffer) -> OMRendererTask *
    {
        bindIndirectBuffer(buffer);
        return this;
    }
    virtual void bindTarget(OMRendererRenderTarget *target) = 0;
    inline auto target(OMRendererRenderTarget *target) -> OMRendererTask *
    {
        bindTarget(target);
        return this;
    }
    virtual void drawInstance(uint64_t vertexCount, uint64_t instanceCount) = 0;
    inline auto drawInstanceN(uint64_t vertexCount, uint64_t instanceCount) -> OMRendererTask *
    {
        drawInstance(vertexCount, instanceCount);
        return this;
    }
    virtual void drawIndexed(uint64_t vertexCount) = 0;
    inline auto drawIndexedN(uint64_t vertexCount) -> OMRendererTask *
    {
        drawIndexed(vertexCount);
        return this;
    }
    virtual void drawIndexedInstance(uint64_t vertexCount, uint64_t instanceCount) = 0;
    inline auto drawIndexedInstanceN(uint64_t vertexCount, uint64_t instanceCount) -> OMRendererTask *
    {
        drawIndexedInstance(vertexCount, instanceCount);
        return this;
    }
    virtual void drawIndexedInstance(uint64_t vertexCount, uint64_t instanceCount, uint64_t firstInstance) = 0;
    inline auto drawIndexedInstanceN(uint64_t vertexCount, uint64_t instanceCount, uint64_t firstInstance)
        -> OMRendererTask *
    {
        drawIndexedInstance(vertexCount, instanceCount, firstInstance);
        return this;
    }
    virtual void drawIndirect(uint64_t begin, uint64_t count) = 0;
    inline auto drawIndirectN(uint64_t begin, uint64_t count) -> OMRendererTask *
    {
        drawIndirect(begin, count);
        return this;
    }
    virtual void finish() = 0;
    inline auto finishN() -> OMRendererTask *
    {
        finish();
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

  protected:
    glm::vec4 colorClear = {0.0f, 0.0f, 0.0f, 0.0f};
    float depthClear = 1.0f;

  private:
    std::vector<OMRendererTask *> dependTasks;
};
} // namespace openminecraft::renderer::common

#endif
