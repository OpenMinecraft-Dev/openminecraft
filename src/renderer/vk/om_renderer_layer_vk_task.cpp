#include "openminecraft/renderer/vk/om_renderer_layer_vk_task.hpp"
#include "openminecraft/renderer/vk/om_renderer_layer_vk.hpp"
#include "openminecraft/renderer/vk/om_renderer_layer_vk_buffer.hpp"
#include "openminecraft/renderer/vk/om_renderer_layer_vk_pipeline.hpp"
#include "openminecraft/renderer/vk/om_renderer_layer_vk_rendertarget.hpp"
#include <iterator>
#include <vulkan/vulkan.hpp>

using namespace ::vk;

namespace openminecraft::renderer::vk
{
OMRendererTaskVk::OMRendererTaskVk(OMRendererVk *renderer) : renderer(renderer), common::OMRendererTask(renderer)
{
    commandBuffer = renderer->logicalDevice.allocateCommandBuffers(
        {renderer->tempCommandPool, CommandBufferLevel::eSecondary, 1})[0];
}
OMRendererTaskVk::~OMRendererTaskVk()
{
    renderer->logicalDevice.freeCommandBuffers(renderer->tempCommandPool, commandBuffer);
}

void OMRendererTaskVk::bindPipeline(common::OMRendererPipeline *pipeline)
{
    commandBuffer.bindPipeline(PipelineBindPoint::eGraphics,
                               reinterpret_cast<OMRendererPipelineVk *>(pipeline)->getPipeline());
    commandBuffer.bindDescriptorSets(PipelineBindPoint::eGraphics,
                                     reinterpret_cast<OMRendererPipelineVk *>(pipeline)->getPipelineLayout(), 0,
                                     reinterpret_cast<OMRendererPipelineVk *>(pipeline)->getDescSet(), nullptr);
}
void OMRendererTaskVk::bindVertexBuffer(std::vector<common::OMRendererBuffer *> buffer)
{
    std::vector<DeviceSize> sizes = {};
    std::vector<Buffer> buffers = {};
    for (auto b : buffer)
    {
        sizes.push_back(0);
        buffers.push_back(reinterpret_cast<OMRendererBufferVk *>(b)->buffer);
    }
    commandBuffer.bindVertexBuffers(0, buffers, sizes);
}
void OMRendererTaskVk::bindIndexBuffer(common::OMRendererBuffer *buffer)
{
    commandBuffer.bindIndexBuffer(reinterpret_cast<OMRendererBufferVk *>(buffer)->buffer, 0, IndexType::eUint32);
}
void OMRendererTaskVk::bindTarget(common::OMRendererRenderTarget *target)
{
    auto ext = target->fetchSize();
    auto ii = CommandBufferInheritanceInfo(reinterpret_cast<OMRendererRenderTargetVk *>(target)->renderPass);
    commandBuffer.begin(
        {CommandBufferUsageFlagBits::eSimultaneousUse | CommandBufferUsageFlagBits::eRenderPassContinue, &ii});
    commandBuffer.setViewport(0, {Viewport(0, 0, ext.x, ext.y, 0, 1)});
    commandBuffer.setScissor(0, {Rect2D(Offset2D(0, 0), Extent2D(ext.x, ext.y))});
}
void OMRendererTaskVk::draw(uint64_t vertexCount)
{
    commandBuffer.drawIndexed(vertexCount, 1, 0, 0, 0);
}
void OMRendererTaskVk::finish()
{
    commandBuffer.end();
}
} // namespace openminecraft::renderer::vk
