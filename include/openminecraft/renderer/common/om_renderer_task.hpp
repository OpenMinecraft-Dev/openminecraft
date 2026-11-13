#ifndef OM_RENDERER_TASK_HPP
#define OM_RENDERER_TASK_HPP

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
// Matches Vulkan's VkDrawIndexedIndirectCommand and OpenGL's
// DrawElementsIndirectCommand (5 uint32_t, tightly packed).
struct OMRendererDrawIndexedIndirect
{
    uint32_t indexCount = 0;
    uint32_t instanceCount = 0;
    uint32_t firstIndex = 0;
    uint32_t vertexOffset = 0;
    uint32_t firstInstance = 0;
};

class OMRendererTask : public OMRendererObject
{
  public:
    OMRendererTask(OMRenderer *renderer)
    {
    }

    virtual ~OMRendererTask() = default;

    inline void setClearColor(glm::vec4 c, int i = 0)
    {
        colorClear.resize(i + 1);
        colorClear[i] = c;
    }
    inline auto clearColor(glm::vec4 color, int i = 0) -> OMRendererTask *
    {
        setClearColor(color, i);
        return this;
    }
    inline auto removeClearColor(int i) -> OMRendererTask *
    {
        colorClear.erase(colorClear.begin() + i);
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

    // Restricts subsequent draws to a rectangle in target pixels (top-left origin).
    // Backends record this dynamically inside the task; no pipeline/target changes.
    virtual void setScissor(float x, float y, float width, float height)
    {
    }
    inline auto scissor(float x, float y, float width, float height) -> OMRendererTask *
    {
        setScissor(x, y, width, height);
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

    // Whether this backend/task supports indirect draw. Vulkan always returns
    // true; OpenGL decides inside its task implementation from the runtime GL
    // version/function availability.
    virtual auto isSupportIndirect() const -> bool
    {
        return false;
    }

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
    virtual void drawInstance(uint64_t vertexCount, uint64_t instanceCount, uint64_t firstInstance) = 0;
    inline auto drawInstanceN(uint64_t vertexCount, uint64_t instanceCount, uint64_t firstInstance) -> OMRendererTask *
    {
        drawInstance(vertexCount, instanceCount, firstInstance);
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
    virtual void drawIndirect() = 0;
    inline auto drawIndirectN() -> OMRendererTask *
    {
        drawIndirect();
        return this;
    }

    // Backend-neutral indirect draw with a CPU fallback. The decision belongs
    // to the task: Vulkan takes the indirect path, while OpenGL uses indirect
    // only when the task reports support and otherwise replays the same
    // commands through normal instanced draws. The fallback path supports
    // firstInstance but assumes firstIndex == 0 and vertexOffset == 0 (the
    // voxel index buffer layout).
    inline auto drawIndexedIndirectN(OMRendererBuffer *indirect, uint64_t begin, uint64_t count,
                                     const std::vector<OMRendererDrawIndexedIndirect> &fallback)
        -> OMRendererTask *
    {
        if (isSupportIndirect())
        {
            bindIndirectBuffer(indirect);
            drawIndirect(begin, count);
            return this;
        }

        for (const auto &cmd : fallback)
        {
            if (cmd.instanceCount == 0 || cmd.indexCount == 0)
            {
                continue;
            }

            drawIndexedInstance(cmd.indexCount, cmd.instanceCount, cmd.firstInstance);
        }

        return this;
    }
    virtual void finish() = 0;
    inline auto finishN() -> OMRendererTask *
    {
        finish();
        return this;
    }
    virtual void resolveTo(OMRendererRenderTarget *target) = 0;
    inline auto resolve(OMRendererRenderTarget *target) -> OMRendererTask *
    {
        resolveTo(target);
        return this;
    }
    virtual void pushDebugTag(std::string) = 0;
    inline auto beginDebugTag(std::string s) -> OMRendererTask *
    {
        pushDebugTag(s);
        return this;
    }
    virtual void popDebugTag() = 0;
    inline auto endDebugTag() -> OMRendererTask *
    {
        popDebugTag();
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

    // INFO: use setters instead of direct assignment so external code cannot freely mutate task state
    inline auto setDisabled(bool value) -> OMRendererTask *
    {
        disabledState = value;
        return this;
    }

    inline auto isDisabled() const -> bool
    {
        return disabledState;
    }

  protected:
    std::vector<glm::vec4> colorClear = {{0.0f, 0.0f, 0.0f, 0.0f}};
    float depthClear = 1.0f;

  private:
    bool disabledState = false;
    std::vector<OMRendererTask *> dependTasks;
};
} // namespace openminecraft::renderer::common

#endif
