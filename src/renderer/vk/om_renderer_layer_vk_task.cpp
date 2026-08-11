#include "openminecraft/renderer/vk/om_renderer_layer_vk_task.hpp"
#include "openminecraft/i18n/om_i18n_res.hpp"
#include "openminecraft/renderer/common/om_renderer_buffer.hpp"
#include "openminecraft/renderer/common/om_renderer_texture.hpp"
#include "openminecraft/renderer/om_renderer_exception.hpp"
#include "openminecraft/renderer/vk/om_renderer_layer_vk.hpp"
#include "openminecraft/renderer/vk/om_renderer_layer_vk_buffer.hpp"
#include "openminecraft/renderer/vk/om_renderer_layer_vk_pipeline.hpp"
#include "openminecraft/renderer/vk/om_renderer_layer_vk_rendertarget.hpp"
#include "openminecraft/renderer/vk/om_renderer_layer_vk_texture.hpp"
#include "vulkan/vulkan.hpp"
#include "vulkan/vulkan_enums.hpp"
#include "vulkan/vulkan_structs.hpp"
#include <cstdint>
#include <stdexcept>
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
    pipe = pipeline;
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
void OMRendererTaskVk::bindIndirectBuffer(common::OMRendererBuffer *buffer)
{
    indirectBuffer = buffer;
}
void OMRendererTaskVk::drawIndirect(uint64_t begin, uint64_t count)
{
    try
    {
        commandBuffer.drawIndexedIndirect(reinterpret_cast<OMRendererBufferVk *>(indirectBuffer)->buffer,
                                          static_cast<DeviceSize>(begin * 5 * sizeof(uint32_t)), count,
                                          5 * sizeof(uint32_t));
    }
    catch (SystemError &e)
    {
        throw OMRendererException(VkErrorTranslate(e, "openminecraft.renderer.vk.err.task"));
    }
}
void OMRendererTaskVk::bindTarget(common::OMRendererRenderTarget *target)
{
    this->target = target;
    try
    {
        if (target == renderer->getDefaultRenderTarget())
        {
            isDefault = true;
        }
        auto ext = target->fetchSize();
        renderer->logicalDevice.waitIdle();
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
                    test.push_back(ClearValue({depthClear, 0}));
                }
                else
                {
                    test.push_back(ClearValue({colorClear.r, colorClear.g, colorClear.b, colorClear.a}));
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
void OMRendererTaskVk::drawInstance(uint64_t vertexCount, uint64_t instanceCount)
{
    try
    {
        commandBuffer.draw(vertexCount, instanceCount, 0, 0);
    }
    catch (SystemError &e)
    {
        throw OMRendererException(VkErrorTranslate(e, "openminecraft.renderer.vk.err.task"));
    }
}
void OMRendererTaskVk::drawIndexed(uint64_t vertexCount)
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
void OMRendererTaskVk::drawIndexedInstance(uint64_t vertexCount, uint64_t instanceCount)
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
void OMRendererTaskVk::drawIndexedInstance(uint64_t vertexCount, uint64_t instanceCount, uint64_t firstInstance)
{
    try
    {
        commandBuffer.drawIndexed(vertexCount, instanceCount, 0, 0, firstInstance);
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
        if (!isDefault && !isResolved)
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
void OMRendererTaskVk::resolveTo(common::OMRendererRenderTarget *target)
{
    try
    {
        commandBuffer.endRenderPass();
        isResolved = true;

        auto src = reinterpret_cast<OMRendererRenderTargetVk *>(this->target);
        auto dst = reinterpret_cast<OMRendererRenderTargetVk *>(target);

        if (src->textures.size() != dst->textures.size())
        {
            throw std::logic_error("fail!");
        }

        for (int i = 0; i < src->textures.size(); ++i)
        {
            if (src->textures[i]->arr != common::Depth && dst->textures[i]->arr != common::Depth)
            {
                auto srci = reinterpret_cast<OMRendererTextureVk *>(src->textures[i]);
                auto dsti = reinterpret_cast<OMRendererTextureVk *>(dst->textures[i]);

                srci->transitionImageLayout(commandBuffer, ImageLayout::eShaderReadOnlyOptimal,
                                            ImageLayout::eTransferSrcOptimal, 0, 0, 1);
                dsti->transitionImageLayout(commandBuffer, ImageLayout::eUndefined, ImageLayout::eTransferDstOptimal, 0,
                                            0, 1);

                commandBuffer.resolveImage(
                    srci->image, ImageLayout::eTransferSrcOptimal, dsti->image, ImageLayout::eTransferDstOptimal,
                    {ImageResolve(ImageSubresourceLayers(ImageAspectFlagBits::eColor, 0, 0, 1), Offset3D(0, 0, 0),
                                  ImageSubresourceLayers(ImageAspectFlagBits::eColor, 0, 0, 1), Offset3D(0, 0, 0),
                                  Extent3D(srci->width, srci->height, 1))});

                srci->transitionImageLayout(commandBuffer, ImageLayout::eTransferSrcOptimal,
                                            ImageLayout::eShaderReadOnlyOptimal, 0, 0, 1);
                dsti->transitionImageLayout(commandBuffer, ImageLayout::eTransferDstOptimal,
                                            ImageLayout::eShaderReadOnlyOptimal, 0, 0, 1);
            }
        }
    }
    catch (SystemError &e)
    {
        throw OMRendererException(VkErrorTranslate(e, "openminecraft.renderer.vk.err.task"));
    }
}
} // namespace openminecraft::renderer::vk
