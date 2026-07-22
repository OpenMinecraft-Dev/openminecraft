#ifndef OM_RENDERER_LAYER_VK_TASK_HPP
#define OM_RENDERER_LAYER_VK_TASK_HPP

#include "openminecraft/renderer/common/om_renderer_task.hpp"
#include "openminecraft/renderer/vk/om_renderer_layer_vk.hpp"

namespace openminecraft::renderer::vk
{
class OMRendererTaskVk : public common::OMRendererTask
{
  public:
    OMRendererTaskVk(OMRendererVk *renderer);
    ~OMRendererTaskVk() override;

    void bindPipeline(common::OMRendererPipeline *pipeline) override;
    void bindVertexBuffer(std::vector<common::OMRendererBuffer *> buffer) override;
    void bindIndexBuffer(common::OMRendererBuffer *buffer) override;
    void bindTarget(common::OMRendererRenderTarget *target) override;
    void draw(uint64_t vertexCount) override;
    void drawInstance(uint64_t vertexCount, uint64_t instanceCount) override;
    void finish() override;
    void clear() override;

    auto isOnDefault() -> bool;

    ::vk::CommandBuffer commandBuffer;

  private:
    OMRendererVk *renderer;
    bool isDefault = false;
    bool begin = false;
};
}; // namespace openminecraft::renderer::vk

#endif
