#include "openminecraft/renderer/vk/om_renderer_layer_vk_rendertarget.hpp"
#include "openminecraft/i18n/om_i18n_res.hpp"
#include "openminecraft/renderer/common/om_renderer_rendertarget.hpp"
#include "openminecraft/renderer/common/om_renderer_texture.hpp"
#include "openminecraft/renderer/om_renderer_exception.hpp"
#include "openminecraft/renderer/vk/om_renderer_layer_vk.hpp"
#include "openminecraft/renderer/vk/om_renderer_layer_vk_texture.hpp"
#include "vulkan/vulkan.hpp"
#include <algorithm>
#include <iostream>
#include <stdexcept>

using namespace ::vk;
using namespace openminecraft::i18n::res;

namespace openminecraft::renderer::vk
{
OMRendererRenderTargetVk::OMRendererRenderTargetVk(OMRendererVk *renderer)
    : renderer(renderer), common::OMRendererRenderTarget(renderer)
{
}

OMRendererRenderTargetVk::~OMRendererRenderTargetVk()
{
    try
    {
        if (block)
        {
            renderer->logicalDevice.destroyFramebuffer(block->framebuffer, renderer->allocator);
            delete block;
        }
        if (available)
        {
            renderer->logicalDevice.destroyRenderPass(renderPass, renderer->allocator);
        }
    }
    catch (SystemError &e)
    {
    }
}

void OMRendererRenderTargetVk::attachTarget(common::OMRendererTexture *texture)
{
    textures.push_back(texture);
}

glm::vec2 OMRendererRenderTargetVk::fetchSize()
{
    if (textures.empty())
    {
        return {renderer->swapchainManager->extent.width, renderer->swapchainManager->extent.height};
    }

    glm::vec2 target = {0.0f, 0.0f};
    for (auto tex : textures)
    {
        auto tt = reinterpret_cast<OMRendererTextureVk *>(tex);
        target.x = std::max(target.x, static_cast<float>(tt->width));
        target.y = std::max(target.y, static_cast<float>(tt->height));
    }
    return target;
}

void OMRendererRenderTargetVk::build()
{
    if (available)
    {
        return;
    }

    try
    {
        // gino: no textures, render it to the screen (default properties)
        if (textures.empty())
        {
            auto attaches = std::vector{AttachmentReference(0, ImageLayout::eColorAttachmentOptimal)};
            auto depthAtt = AttachmentReference(1, ImageLayout::eDepthStencilAttachmentOptimal);

            auto attachments = std::vector{
                AttachmentDescription({}, renderer->swapchainManager->format.format, SampleCountFlagBits::e1,
                                      AttachmentLoadOp::eClear, AttachmentStoreOp::eStore, AttachmentLoadOp::eDontCare,
                                      AttachmentStoreOp::eDontCare, ImageLayout::eUndefined,
                                      ImageLayout::ePresentSrcKHR),
                AttachmentDescription({}, Format::eD32Sfloat, SampleCountFlagBits::e1, AttachmentLoadOp::eClear,
                                      AttachmentStoreOp::eDontCare, AttachmentLoadOp::eDontCare,
                                      AttachmentStoreOp::eDontCare, ImageLayout::eUndefined,
                                      ImageLayout::eDepthStencilAttachmentOptimal)};
            auto subpasses =
                std::vector{SubpassDescription({}, PipelineBindPoint::eGraphics, nullptr, attaches, {}, &depthAtt)};
            auto depe = std::vector{SubpassDependency(
                VK_SUBPASS_EXTERNAL, 0,
                PipelineStageFlagBits::eColorAttachmentOutput | PipelineStageFlagBits::eEarlyFragmentTests,
                PipelineStageFlagBits::eColorAttachmentOutput | PipelineStageFlagBits::eEarlyFragmentTests, {},
                AccessFlagBits::eColorAttachmentRead | AccessFlagBits::eColorAttachmentWrite |
                    AccessFlagBits::eDepthStencilAttachmentWrite)};

            renderPass = renderer->logicalDevice.createRenderPass(
                RenderPassCreateInfo({}, attachments, subpasses, depe), renderer->allocator);
        }
        else
        {
            std::vector<AttachmentReference> colorAttach, depthAttach = {};
            std::vector<AttachmentDescription> attachDesc = {};
            uint32_t a = 0;
            for (auto tt : textures)
            {
                if (tt->arr == common::OMTextureArrangement::Depth)
                {
                    attachDesc.push_back({{},
                                          reinterpret_cast<OMRendererTextureVk *>(tt)->format,
                                          SampleCountFlagBits::e1,
                                          AttachmentLoadOp::eClear,
                                          AttachmentStoreOp::eDontCare,
                                          AttachmentLoadOp::eDontCare,
                                          AttachmentStoreOp::eDontCare,
                                          ImageLayout::eUndefined,
                                          ImageLayout::eDepthStencilAttachmentOptimal});
                    depthAttach.push_back({a, ImageLayout::eDepthStencilAttachmentOptimal});
                }
                else
                {
                    attachDesc.push_back({{},
                                          reinterpret_cast<OMRendererTextureVk *>(tt)->format,
                                          SampleCountFlagBits::e1,
                                          AttachmentLoadOp::eClear,
                                          AttachmentStoreOp::eStore,
                                          AttachmentLoadOp::eDontCare,
                                          AttachmentStoreOp::eDontCare,
                                          ImageLayout::eUndefined,
                                          ImageLayout::eShaderReadOnlyOptimal});
                    colorAttach.push_back({a, ImageLayout::eColorAttachmentOptimal});
                }
                a++;
            }

            if (depthAttach.size() > 1)
            {
                throw OMRendererException(i18n::res::translate("openminecraft.renderer.vk.err.fb.depth"));
            }

            auto subpasses = std::vector{SubpassDescription({}, PipelineBindPoint::eGraphics, nullptr, colorAttach, {},
                                                            depthAttach.empty() ? nullptr : depthAttach.data())};
            auto depe = std::vector{SubpassDependency(
                VK_SUBPASS_EXTERNAL, 0,
                PipelineStageFlagBits::eColorAttachmentOutput | PipelineStageFlagBits::eEarlyFragmentTests,
                PipelineStageFlagBits::eColorAttachmentOutput | PipelineStageFlagBits::eEarlyFragmentTests, {},
                AccessFlagBits::eColorAttachmentRead | AccessFlagBits::eColorAttachmentWrite |
                    AccessFlagBits::eDepthStencilAttachmentWrite)};

            renderPass = renderer->logicalDevice.createRenderPass(RenderPassCreateInfo({}, attachDesc, subpasses, depe),
                                                                  renderer->allocator);

            block = new OMRendererRenderTargetBlock;
            buildFramebuffer();
        }
        available = true;
    }
    catch (SystemError &e)
    {
        throw OMRendererException(VkErrorTranslate(e, "openminecraft.renderer.vk.err.renderpass"));
    }
}

void OMRendererRenderTargetVk::replaceTarget(int idx, common::OMRendererTexture *texture)
{
    textures[idx] = texture;
}

void OMRendererRenderTargetVk::rebuild()
{
    renderer->logicalDevice.destroyFramebuffer(block->framebuffer, renderer->allocator);
    buildFramebuffer();
}

void OMRendererRenderTargetVk::buildFramebuffer()
{
    std::vector<ImageView> attch;
    for (auto tt : textures)
    {
        attch.push_back(reinterpret_cast<OMRendererTextureVk *>(tt)->imageView);
    }

    auto ext = fetchSize();
    block->framebuffer = renderer->logicalDevice.createFramebuffer(
        FramebufferCreateInfo({}, renderPass, attch, ext.x, ext.y, 1), renderer->allocator);
}

} // namespace openminecraft::renderer::vk
