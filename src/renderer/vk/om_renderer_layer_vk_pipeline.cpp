#include "openminecraft/renderer/vk/om_renderer_layer_vk_pipeline.hpp"
#include "openminecraft/i18n/om_i18n_res.hpp"
#include "openminecraft/renderer/common/basics/om_vertex_format.hpp"
#include "openminecraft/renderer/common/om_renderer_pipeline.hpp"
#include "openminecraft/renderer/common/om_renderer_shader.hpp"
#include "openminecraft/renderer/vk/om_renderer_layer_vk.hpp"
#include "openminecraft/renderer/vk/om_renderer_layer_vk_texture.hpp"
#include "vulkan/vulkan.hpp"
#include <stdexcept>

using openminecraft::i18n::res::translate;
using namespace ::vk;

namespace openminecraft::renderer::vk
{
OMRendererPipelineVk::OMRendererPipelineVk(OMRendererVk *renderer)
    : renderer(renderer), common::OMRendererPipeline(renderer)
{
}

OMRendererPipelineVk::~OMRendererPipelineVk()
{
    renderer->logicalDevice.destroyRenderPass(renderPass, renderer->allocator);
}

void OMRendererPipelineVk::attachShader(std::shared_ptr<common::OMShader> shader)
{
    auto fin = shader;
    if (fin->type != common::OMShaderFileType::SPIRVBinary)
    {
        fin = fin->convertTo(common::OMShaderFileType::GLSLSource);
    }
    if (fin == nullptr)
    {
        throw std::logic_error(translate("openminecraft.renderer.vk.err.shaderstat"));
    }
    shaders.push_back(renderer->logicalDevice.createShaderModule(ShaderModuleCreateInfo(
        {}, shader->data.size(), reinterpret_cast<const uint32_t *>(shader->data.data()), renderer->allocator)));
}

void OMRendererPipelineVk::vertexFormat(common::basics::OMVertexFormat format)
{
    this->format = format;
}

void OMRendererPipelineVk::bindTarget(std::shared_ptr<common::OMRendererTexture> texture)
{
    this->target = texture;
}

void OMRendererPipelineVk::build()
{
    {
        auto attaches = std::vector{AttachmentReference(0, ImageLayout::eColorAttachmentOptimal)};
        auto depthAtt = AttachmentReference(1, ImageLayout::eDepthStencilAttachmentOptimal);

        auto attachments =
            std::vector{AttachmentDescription(
                            {},
                            (target != nullptr ? reinterpret_cast<OMRendererTextureVk *>(target.get())->format
                                               : renderer->swapchainManager->format.format),
                            SampleCountFlagBits::e1, AttachmentLoadOp::eClear, AttachmentStoreOp::eStore,
                            AttachmentLoadOp::eDontCare, AttachmentStoreOp::eDontCare, ImageLayout::eUndefined,
                            (target != nullptr ? ImageLayout::eShaderReadOnlyOptimal : ImageLayout::ePresentSrcKHR)),
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

    inited = true;
}

} // namespace openminecraft::renderer::vk
