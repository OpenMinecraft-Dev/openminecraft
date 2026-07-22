#include "openminecraft/renderer/vk/om_renderer_layer_vk_task.hpp"
#include "openminecraft/i18n/om_i18n_res.hpp"
#include "openminecraft/renderer/common/om_renderer_texture.hpp"
#include "openminecraft/renderer/om_renderer_exception.hpp"
#include "openminecraft/renderer/vk/om_renderer_layer_vk.hpp"
#include "openminecraft/renderer/vk/om_renderer_layer_vk_buffer.hpp"
#include "openminecraft/renderer/vk/om_renderer_layer_vk_pipeline.hpp"
#include "openminecraft/renderer/vk/om_renderer_layer_vk_rendertarget.hpp"
#include "vulkan/vulkan.hpp"
#include "vulkan/vulkan_enums.hpp"
#include <vulkan/vulkan.hpp>

using namespace ::vk;
using namespace openminecraft::i18n::res;

namespace openminecraft::renderer::vk
{
OMRendererTaskVk::OMRendererTaskVk(OMRendererVk *renderer) : renderer(renderer), common::OMRendererTask(renderer)
{
    try
    {
        commandBuffer = renderer->logicalDevice.allocateCommandBuffers(
            {renderer->tempCommandPool, CommandBufferLevel::eSecondary, 1})[0];
    }
    catch (SystemError &e)
    {
        throw OMRendererException(VkErrorTranslate(e, "openminecraft.renderer.vk.err.task"));
    }
}
OMRendererTaskVk::~OMRendererTaskVk()
{
    try
    {
        renderer->logicalDevice.freeCommandBuffers(renderer->tempCommandPool, commandBuffer);
    }
    catch (SystemError &e)
    {
    }
}

auto OMRendererTaskVk::isOnDefault() -> bool
{
    return isDefault;
}

void OMRendererTaskVk::bindPipeline(common::OMRendererPipeline *pipeline)
{
    try
    {
        commandBuffer.bindPipeline(PipelineBindPoint::eGraphics,
                                   reinterpret_cast<OMRendererPipelineVk *>(pipeline)->getPipeline());
        if (reinterpret_cast<OMRendererPipelineVk *>(pipeline)->getDescSet())
        {
            commandBuffer.bindDescriptorSets(PipelineBindPoint::eGraphics,
                                             reinterpret_cast<OMRendererPipelineVk *>(pipeline)->getPipelineLayout(), 0,
                                             reinterpret_cast<OMRendererPipelineVk *>(pipeline)->getDescSet(), nullptr);
        }
    }
    catch (SystemError &e)
    {
        throw OMRendererException(VkErrorTranslate(e, "openminecraft.renderer.vk.err.task"));
    }
}
void OMRendererTaskVk::bindVertexBuffer(std::vector<common::OMRendererBuffer *> buffer)
{
    try
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
    catch (SystemError &e)
    {
        throw OMRendererException(VkErrorTranslate(e, "openminecraft.renderer.vk.err.task"));
    }
}
void OMRendererTaskVk::bindIndexBuffer(common::OMRendererBuffer *buffer)
{
    try
    {
        commandBuffer.bindIndexBuffer(reinterpret_cast<OMRendererBufferVk *>(buffer)->buffer, 0, IndexType::eUint32);
    }
    catch (SystemError &e)
    {
        throw OMRendererException(VkErrorTranslate(e, "openminecraft.renderer.vk.err.task"));
    }
}
void OMRendererTaskVk::clear()
{
}
void OMRendererTaskVk::bindTarget(common::OMRendererRenderTarget *target)
{
    try
    {
        if (target == renderer->getDefaultRenderTarget())
        {
            isDefault = true;
        }
        auto ext = target->fetchSize();
        if (isDefault)
        {
            auto ii = CommandBufferInheritanceInfo(reinterpret_cast<OMRendererRenderTargetVk *>(target)->renderPass);

            if (!begin)
            {
                commandBuffer.begin(
                    {CommandBufferUsageFlagBits::eSimultaneousUse | CommandBufferUsageFlagBits::eRenderPassContinue,
                     &ii});
                begin = true;
            }
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

            if (!begin)
            {
                commandBuffer.begin({CommandBufferUsageFlagBits::eSimultaneousUse});
                begin = true;
            }
            commandBuffer.beginRenderPass(
                RenderPassBeginInfo(reinterpret_cast<OMRendererRenderTargetVk *>(target)->renderPass,
                                    reinterpret_cast<OMRendererRenderTargetVk *>(target)->block->framebuffer,
                                    Rect2D(Offset2D(0, 0), Extent2D(ext.x, ext.y)), test),
                SubpassContents::eInline);
        }
        commandBuffer.setViewport(0, {Viewport(0, 0, ext.x, ext.y, 0, 1)});
        commandBuffer.setScissor(0, {Rect2D(Offset2D(0, 0), Extent2D(ext.x, ext.y))});
    }
    catch (SystemError &e)
    {
        throw OMRendererException(VkErrorTranslate(e, "openminecraft.renderer.vk.err.task"));
    }
}
void OMRendererTaskVk::draw(uint64_t vertexCount)
{
    try
    {
        commandBuffer.drawIndexed(vertexCount, 1, 0, 0, 0);
    }
    catch (SystemError &e)
    {
        throw OMRendererException(VkErrorTranslate(e, "openminecraft.renderer.vk.err.task"));
    }
}
void OMRendererTaskVk::drawInstance(uint64_t vertexCount, uint64_t instanceCount)
{
    try
    {
        commandBuffer.drawIndexed(vertexCount, instanceCount, 0, 0, 0);
    }
    catch (SystemError &e)
    {
        throw OMRendererException(VkErrorTranslate(e, "openminecraft.renderer.vk.err.task"));
    }
}
void OMRendererTaskVk::finish()
{
    try
    {
        if (!isDefault)
        {
            commandBuffer.endRenderPass();
        }
        commandBuffer.end();
        begin = false;
    }
    catch (SystemError &e)
    {
        throw OMRendererException(VkErrorTranslate(e, "openminecraft.renderer.vk.err.task"));
    }
}
} // namespace openminecraft::renderer::vk
