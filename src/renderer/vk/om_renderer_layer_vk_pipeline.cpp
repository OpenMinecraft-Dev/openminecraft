#include "openminecraft/renderer/vk/om_renderer_layer_vk_pipeline.hpp"
#include "openminecraft/i18n/om_i18n_res.hpp"
#include "openminecraft/renderer/common/basics/om_vertex_format.hpp"
#include "openminecraft/renderer/common/om_renderer_pipeline.hpp"
#include "openminecraft/renderer/common/om_renderer_shader.hpp"
#include "openminecraft/renderer/om_renderer_exception.hpp"
#include "openminecraft/renderer/vk/om_renderer_layer_vk.hpp"
#include "openminecraft/renderer/vk/om_renderer_layer_vk_buffer.hpp"
#include "openminecraft/renderer/vk/om_renderer_layer_vk_rendertarget.hpp"
#include "openminecraft/renderer/vk/om_renderer_layer_vk_texture.hpp"
#include "vulkan/vulkan.hpp"
#include <stdexcept>

using openminecraft::i18n::res::translate;
using namespace ::vk;

namespace openminecraft::renderer::vk
{
OMRendererPipelineVk::OMRendererPipelineVk(OMRendererVk *renderer)
    : renderer(renderer), common::OMRendererPipeline(renderer), logger("OMRendererPipelineVk", this)
{
}

OMRendererPipelineVk::~OMRendererPipelineVk()
{
    try
    {
        if (available)
        {
            for (auto smp : tempSamplers)
            {
                renderer->logicalDevice.destroySampler(smp, renderer->allocator);
            }
            renderer->logicalDevice.freeDescriptorSets(descriptorPool, descriptorSet);
            renderer->logicalDevice.destroyDescriptorPool(descriptorPool, renderer->allocator);
            renderer->logicalDevice.destroyDescriptorSetLayout(descriptorSetLayout, renderer->allocator);
            renderer->logicalDevice.destroyPipeline(pipeline, renderer->allocator);
            renderer->logicalDevice.destroyPipelineLayout(pipelineLayout, renderer->allocator);
        }
    }
    catch (SystemError &e)
    {
        throw OMRendererException(VkErrorTranslate(e, "openminecraft.renderer.vk.err.cleanup"));
    }
}

ShaderStageFlagBits OMRendererPipelineVk::convertTo(common::OMShaderType type)
{
    switch (type)
    {
    case common::Vertex:
        return ShaderStageFlagBits::eVertex;
    default:
    case common::Fragment:
        return ShaderStageFlagBits::eFragment;
    case common::Geometry:
        return ShaderStageFlagBits::eGeometry;
    case common::Compute:
        return ShaderStageFlagBits::eCompute;
    case common::TessControl:
        return ShaderStageFlagBits::eTessellationControl;
    case common::TessEvaluation:
        return ShaderStageFlagBits::eTessellationEvaluation;
    case common::Callable:
        return ShaderStageFlagBits::eCallableKHR;
    case common::AnyHit:
        return ShaderStageFlagBits::eAnyHitKHR;
    case common::RayGen:
        return ShaderStageFlagBits::eRaygenKHR;
    case common::ClosestHit:
        return ShaderStageFlagBits::eClosestHitKHR;
    case common::Miss:
        return ShaderStageFlagBits::eMissKHR;
    case common::Intersection:
        return ShaderStageFlagBits::eIntersectionKHR;
    }
}

void OMRendererPipelineVk::attachShader(std::shared_ptr<common::OMShader> shader)
{
    auto fin = shader;
    if (fin->type != common::OMShaderFileType::SPIRVBinary)
    {
        fin = fin->convertTo(common::OMShaderFileType::SPIRVBinary);
    }
    if (fin == nullptr)
    {
        throw OMRendererException(translate("openminecraft.renderer.vk.err.shaderstat"));
    }
    compiledShaders.push_back(fin);

    auto sm = renderer->logicalDevice.createShaderModule(
        ShaderModuleCreateInfo({}, fin->data.size(), reinterpret_cast<const uint32_t *>(fin->data.data())),
        renderer->allocator);
    shaders.push_back(sm);
    shaderCreateInfos.push_back({{}, convertTo(shader->typebase), sm, fin->entrypoint.c_str()});
}

Format OMRendererPipelineVk::convertTo(common::basics::OMVertexPropType type)
{
    switch (type)
    {
    default:
    case common::basics::Float:
        return Format::eR32Sfloat;
    case common::basics::Vec2f:
        return Format::eR32G32Sfloat;
    case common::basics::Vec3f:
        return Format::eR32G32B32Sfloat;
    case common::basics::Vec4f:
        return Format::eR32G32B32A32Sfloat;
    case common::basics::Double:
        return Format::eR64Sfloat;
    case common::basics::Vec2d:
        return Format::eR64G64Sfloat;
    case common::basics::Vec3d:
        return Format::eR64G64B64Sfloat;
    case common::basics::Vec4d:
        return Format::eR64G64B64A64Sfloat;
    case common::basics::Integer:
        return Format::eR32Sint;
    case common::basics::Vec2i:
        return Format::eR32G32Sint;
    case common::basics::Vec3i:
        return Format::eR32G32B32Sint;
    case common::basics::Vec4i:
        return Format::eR32G32B32A32Sint;
    }
}

void OMRendererPipelineVk::appendInput(common::OMRendererPipelineInputType type)
{
    auto typ = (type == common::ImageSampler ? DescriptorType::eCombinedImageSampler : DescriptorType::eUniformBuffer);
    descriptorSetLayoutBindings.push_back(
        DescriptorSetLayoutBinding(static_cast<uint32_t>(layoutBinding), typ, 1, ShaderStageFlagBits::eAll));
    descriptorPoolSizes.push_back({typ, 1});
    layoutBinding++;
}

void OMRendererPipelineVk::vertexFormat(common::basics::OMVertexFormat format)
{
    vertexInputAttrDesc.clear();
    vertexInputBindingDesc.clear();

    for (auto &p : format.parts)
    {
        vertexInputBindingDesc.push_back({static_cast<uint32_t>(p.binding), static_cast<uint32_t>(p.size),
                                          p.isInstance ? VertexInputRate::eInstance : VertexInputRate::eVertex});
        uint32_t loc = 0;
        for (auto &pp : p.parts)
        {
            vertexInputAttrDesc.push_back({loc, static_cast<uint32_t>(p.binding),
                                           convertTo(std::get<common::basics::OMVertexPropType>(pp)),
                                           static_cast<uint32_t>(std::get<int>(pp))});
            loc++;
        }
    }
}

void OMRendererPipelineVk::bindOutput(common::OMRendererRenderTarget *target)
{
    this->target = target;
}

void OMRendererPipelineVk::bindInput(int idx, common::OMRendererBuffer *buff)
{
    std::vector c = {DescriptorBufferInfo(reinterpret_cast<OMRendererBufferVk *>(buff)->buffer, 0,
                                          static_cast<DeviceSize>(buff->length))};
    renderer->logicalDevice.updateDescriptorSets(
        WriteDescriptorSet(descriptorSet, idx, 0, DescriptorType::eUniformBuffer, {}, c), nullptr);
}
void OMRendererPipelineVk::bindInput(int idx, common::OMRendererTexture *texture)
{
    auto prop = renderer->physicalDevice.getProperties();
    auto fea = renderer->physicalDevice.getFeatures();

    auto textureSampler = renderer->logicalDevice.createSampler(
        SamplerCreateInfo({}, Filter::eLinear, Filter::eLinear, SamplerMipmapMode::eLinear, SamplerAddressMode::eRepeat,
                          SamplerAddressMode::eRepeat, SamplerAddressMode::eRepeat, 0.0f, fea.samplerAnisotropy,
                          prop.limits.maxSamplerAnisotropy, false, CompareOp::eAlways, 0.0f, 0.0f,
                          BorderColor::eIntOpaqueBlack, false),
        renderer->allocator);
    tempSamplers.push_back(textureSampler);

    auto cc = DescriptorImageInfo(textureSampler, reinterpret_cast<OMRendererTextureVk *>(texture)->imageView,
                                  ImageLayout::eShaderReadOnlyOptimal);
    renderer->logicalDevice.updateDescriptorSets(
        WriteDescriptorSet(descriptorSet, idx, 0, DescriptorType::eCombinedImageSampler, cc, {}), nullptr);
}

void OMRendererPipelineVk::build()
{
    if (available)
    {
        return;
    }

    descriptorSetLayout = renderer->logicalDevice.createDescriptorSetLayout(
        DescriptorSetLayoutCreateInfo({}, descriptorSetLayoutBindings), renderer->allocator);

    descriptorPool = renderer->logicalDevice.createDescriptorPool(
        DescriptorPoolCreateInfo(DescriptorPoolCreateFlagBits::eFreeDescriptorSet, descriptorSetLayoutBindings.size(),
                                 descriptorPoolSizes),
        renderer->allocator);
    descriptorSet = renderer->logicalDevice.allocateDescriptorSets({descriptorPool, 1, &descriptorSetLayout})[0];

    pipelineLayout = renderer->logicalDevice.createPipelineLayout(PipelineLayoutCreateInfo({}, descriptorSetLayout),
                                                                  renderer->allocator);

    auto vertexInput = PipelineVertexInputStateCreateInfo({}, vertexInputBindingDesc, vertexInputAttrDesc);
    auto inputAssembly = PipelineInputAssemblyStateCreateInfo({}, PrimitiveTopology::eTriangleList, false);
    auto rasterization = PipelineRasterizationStateCreateInfo(
        {}, false, false, PolygonMode::eFill, CullModeFlagBits::eNone, FrontFace::eCounterClockwise, true, 0, 0, 0, 1);
    auto multisample = PipelineMultisampleStateCreateInfo({}, SampleCountFlagBits::e1, false);
    auto viewportState = PipelineViewportStateCreateInfo({}, 1, nullptr, 1, nullptr);
    std::vector attc = {PipelineColorBlendAttachmentState(false, {}, {}, {}, {}, {}, {},
                                                          ColorComponentFlagBits::eA | ColorComponentFlagBits::eR |
                                                              ColorComponentFlagBits::eG | ColorComponentFlagBits::eB)};
    auto colorblend = PipelineColorBlendStateCreateInfo({}, true, LogicOp::eCopy, attc, std::array{0.f, 0.f, 0.f, 0.f});
    auto depthStencil =
        PipelineDepthStencilStateCreateInfo({}, true, true, CompareOp::eLess, true, true, {}, {}, 0.0f, 1.0f);
    std::vector<DynamicState> states = {DynamicState::eScissor, DynamicState::eViewport};
    auto dynamicState = PipelineDynamicStateCreateInfo({}, states);

    available = true;

    auto result = renderer->logicalDevice.createGraphicsPipeline(
        {},
        GraphicsPipelineCreateInfo(
            {}, shaderCreateInfos, &vertexInput, &inputAssembly, {}, &viewportState, &rasterization, &multisample,
            &depthStencil, &colorblend, &dynamicState, pipelineLayout,
            reinterpret_cast<OMRendererRenderTargetVk *>(target ? target : renderer->defaultTarget)->renderPass, 0, {},
            -1),
        renderer->allocator);
    if (result.result != Result::eSuccess)
    {
        renderer->logicalDevice.freeDescriptorSets(descriptorPool, descriptorSet);
        renderer->logicalDevice.destroyDescriptorPool(descriptorPool, renderer->allocator);
        renderer->logicalDevice.destroyDescriptorSetLayout(descriptorSetLayout, renderer->allocator);
        renderer->logicalDevice.destroyPipelineLayout(pipelineLayout, renderer->allocator);
        throw OMRendererException(
            VkErrorTranslate(SystemError(result.result), "openminecraft.renderer.vk.err.pipeline"));
    }
    pipeline = result.value;

    for (auto sd : shaders)
    {
        renderer->logicalDevice.destroyShaderModule(sd, renderer->allocator);
    }
}

} // namespace openminecraft::renderer::vk
