#include "openminecraft/renderer/vk/om_renderer_layer_vk_rendertarget.hpp"
#include "openminecraft/renderer/common/om_renderer_rendertarget.hpp"
#include "openminecraft/renderer/common/om_renderer_texture.hpp"
#include "openminecraft/renderer/vk/om_renderer_layer_vk.hpp"
#include "openminecraft/renderer/vk/om_renderer_layer_vk_texture.hpp"
#include "vulkan/vulkan.hpp"
#include <stdexcept>

using namespace ::vk;

namespace openminecraft::renderer::vk
{
OMRendererRenderTargetVk::OMRendererRenderTargetVk(OMRendererVk *renderer)
    : renderer(renderer), common::OMRendererRenderTarget(renderer)
{
}

OMRendererRenderTargetVk::~OMRendererRenderTargetVk()
{
    if (available)
    {
        renderer->logicalDevice.destroyRenderPass(renderPass, renderer->allocator);
    }
}

void OMRendererRenderTargetVk::attachTarget(common::OMRendererTexture *texture)
{
    textures.push_back(texture);
}

void OMRendererRenderTargetVk::build()
{
    if (available)
    {
        return;
    }
    // gino: no textures, render it to the screen (default properties)
    if (textures.empty())
    {
        auto attaches = std::vector{AttachmentReference(0, ImageLayout::eColorAttachmentOptimal)};
        auto depthAtt = AttachmentReference(1, ImageLayout::eDepthStencilAttachmentOptimal);

        auto attachments = std::vector{
            AttachmentDescription({}, renderer->swapchainManager->format.format, SampleCountFlagBits::e1,
                                  AttachmentLoadOp::eClear, AttachmentStoreOp::eStore, AttachmentLoadOp::eDontCare,
                                  AttachmentStoreOp::eDontCare, ImageLayout::eUndefined, ImageLayout::ePresentSrcKHR),
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

        renderPass = renderer->logicalDevice.createRenderPass(RenderPassCreateInfo({}, attachments, subpasses, depe),
                                                              renderer->allocator);
    }
    else
    {
        std::vector<AttachmentReference> colorAttach, depthAttach = {};
        std::vector<AttachmentDescription> attachDesc = {};
        uint32_t a = 0;
        for (auto tt : textures)
        {
            depthAttach.push_back({a, tt->arr == common::OMTextureArrangement::Depth
                                          ? ImageLayout::eDepthStencilAttachmentOptimal
                                          : ImageLayout::eColorAttachmentOptimal});
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
                                      ImageLayout::ePresentSrcKHR});
            }
            a++;
        }

        if (depthAttach.size() > 1)
        {
            throw std::logic_error("too many depth buffers!");
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
    }
    available = true;
}

} // namespace openminecraft::renderer::vk
