#include "openminecraft/renderer/vk/om_renderer_layer_vk_task.hpp"
#include "openminecraft/renderer/common/om_renderer_texture.hpp"
#include "openminecraft/renderer/vk/om_renderer_layer_vk.hpp"
#include "openminecraft/renderer/vk/om_renderer_layer_vk_buffer.hpp"
#include "openminecraft/renderer/vk/om_renderer_layer_vk_pipeline.hpp"
#include "openminecraft/renderer/vk/om_renderer_layer_vk_rendertarget.hpp"
#include "vulkan/vulkan.hpp"
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

bool OMRendererTaskVk::isOnDefault()
{
    return isDefault;
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
    if (target == renderer->getDefaultRenderTarget())
    {
        isDefault = true;
    }
    auto ext = target->fetchSize();
    if (isDefault)
    {
        auto ii = CommandBufferInheritanceInfo(reinterpret_cast<OMRendererRenderTargetVk *>(target)->renderPass);
        commandBuffer.begin(
            {CommandBufferUsageFlagBits::eSimultaneousUse | CommandBufferUsageFlagBits::eRenderPassContinue, &ii});
    }
    else
    {
        renderer->logicalDevice.freeCommandBuffers(renderer->tempCommandPool, commandBuffer);
        commandBuffer = renderer->logicalDevice.allocateCommandBuffers(
            {renderer->tempCommandPool, CommandBufferLevel::ePrimary, 1})[0];

        auto rt = reinterpret_cast<OMRendererRenderTargetVk *>(target);

        std::vector<ClearValue> test;
        for (auto ii : rt->textures)
        {
            if (ii->arr == common::Depth)
            {
                test.push_back(ClearValue({1.0f, 0}));
            }
            else
            {
                test.push_back(ClearValue({0.0f, 0.0f, 0.0f, 0.0f}));
            }
        }

        commandBuffer.begin({CommandBufferUsageFlagBits::eSimultaneousUse});
        commandBuffer.beginRenderPass(
            RenderPassBeginInfo(reinterpret_cast<OMRendererRenderTargetVk *>(target)->renderPass,
                                reinterpret_cast<OMRendererRenderTargetVk *>(target)->block->framebuffer,
                                Rect2D(Offset2D(0, 0), Extent2D(ext.x, ext.y)), test),
            SubpassContents::eInline);
    }
    commandBuffer.setViewport(0, {Viewport(0, 0, ext.x, ext.y, 0, 1)});
    commandBuffer.setScissor(0, {Rect2D(Offset2D(0, 0), Extent2D(ext.x, ext.y))});
}
void OMRendererTaskVk::draw(uint64_t vertexCount)
{
    commandBuffer.drawIndexed(vertexCount, 1, 0, 0, 0);
}
void OMRendererTaskVk::finish()
{
    if (!isDefault)
    {
        commandBuffer.endRenderPass();
    }
    commandBuffer.end();
}
} // namespace openminecraft::renderer::vk
