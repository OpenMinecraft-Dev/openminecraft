#include "openminecraft/renderer/vk/om_renderer_layer_vk_pipeline.hpp"
#include "openminecraft/i18n/om_i18n_res.hpp"
#include "openminecraft/renderer/common/basics/om_vertex_format.hpp"
#include "openminecraft/renderer/common/om_renderer_buffer.hpp"
#include "openminecraft/renderer/common/om_renderer_pipeline.hpp"
#include "openminecraft/renderer/common/om_renderer_shader.hpp"
#include "openminecraft/renderer/om_renderer_exception.hpp"
#include "openminecraft/renderer/vk/om_renderer_layer_vk.hpp"
#include "openminecraft/renderer/vk/om_renderer_layer_vk_buffer.hpp"
#include "openminecraft/renderer/vk/om_renderer_layer_vk_rendertarget.hpp"
#include "openminecraft/renderer/vk/om_renderer_layer_vk_texture.hpp"
#include "vulkan/vulkan.hpp"
#include "vulkan/vulkan_enums.hpp"
#include "vulkan/vulkan_structs.hpp"
#include <chrono>
#include <thread>

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

            for (auto m : tempBufferViews)
            {
                renderer->logicalDevice.destroyBufferView(m, renderer->allocator);
            }

            if (descriptorPool || descriptorSet)
            {
                renderer->logicalDevice.freeDescriptorSets(descriptorPool, descriptorSet);
                renderer->logicalDevice.destroyDescriptorPool(descriptorPool, renderer->allocator);
            }
            renderer->logicalDevice.destroyDescriptorSetLayout(descriptorSetLayout, renderer->allocator);
            renderer->logicalDevice.destroyPipeline(pipeline, renderer->allocator);
            renderer->logicalDevice.destroyPipelineLayout(pipelineLayout, renderer->allocator);
        }
    }
    catch (SystemError &e)
    {
    }
}

auto OMRendererPipelineVk::convertTo(common::OMShaderType type) -> ShaderStageFlagBits
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
    try
    {
        auto fin = shader;
        if (fin->type != common::OMShaderFileType::SPIRVBinary)
        {
            shaderIds.push_back(renderer->compiler.addCompileTask(fin));
            return;
        }
        if (fin == nullptr)
        {
            throw OMRendererException(translate("openminecraft.renderer.vk.err.shaderstat"));
        }

        auto sm = renderer->logicalDevice.createShaderModule(
            ShaderModuleCreateInfo({}, fin->data.size(), reinterpret_cast<const uint32_t *>(fin->data.data())),
            renderer->allocator);
        shaders.push_back(sm);
        shaderCreateInfos.push_back({{}, convertTo(shader->typebase), sm, fin->entrypoint.c_str()});
    }
    catch (SystemError &e)
    {
        throw OMRendererException(VkErrorTranslate(e, "openminecraft.renderer.vk.err.pipeline.shader"));
    }
}

auto OMRendererPipelineVk::convertTo(common::basics::OMVertexPropType type) -> Format
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
    DescriptorType typ;
    switch (type)
    {
    case common::ImageSampler:
        typ = DescriptorType::eCombinedImageSampler;
        break;
    case common::UniformBuffer:
        typ = DescriptorType::eUniformBuffer;
        break;
    case common::ShaderStorageBuffer:
        typ = DescriptorType::eStorageBuffer;
        break;
    case common::UniformTexelBuffer:
        typ = DescriptorType::eUniformTexelBuffer;
        break;
    }
    descriptorSetLayoutBindings.emplace_back(static_cast<uint32_t>(layoutBinding), typ, 1, ShaderStageFlagBits::eAll);
    descriptorPoolSizes.emplace_back(typ, 1);
    layoutBinding++;
}

void OMRendererPipelineVk::vertexFormat(common::basics::OMVertexFormat format)
{
    vertexInputAttrDesc.clear();
    vertexInputBindingDesc.clear();

    uint32_t loc = 0;
    for (auto &p : format.parts)
    {
        vertexInputBindingDesc.emplace_back(static_cast<uint32_t>(p.binding), static_cast<uint32_t>(p.size),
                                            p.isInstance ? VertexInputRate::eInstance : VertexInputRate::eVertex);
        for (auto &pp : p.parts)
        {
            vertexInputAttrDesc.emplace_back(loc, static_cast<uint32_t>(p.binding),
                                             convertTo(std::get<common::basics::OMVertexPropType>(pp)),
                                             static_cast<uint32_t>(std::get<int>(pp)));
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
    try
    {
        if (buff->usage == common::UniformTexel)
        {
            auto bv = renderer->logicalDevice.createBufferView(
                BufferViewCreateInfo({}, reinterpret_cast<OMRendererBufferVk *>(buff)->buffer, Format::eR32Sfloat, 0,
                                     static_cast<DeviceSize>(buff->length)));
            tempBufferViews.push_back(bv);
            renderer->logicalDevice.updateDescriptorSets(
                WriteDescriptorSet(descriptorSet, idx, 0, descriptorSetLayoutBindings[idx].descriptorType, {}, {},
                                   {bv}),
                nullptr);
            return;
        }
        std::vector c = {DescriptorBufferInfo(reinterpret_cast<OMRendererBufferVk *>(buff)->buffer, 0,
                                              static_cast<DeviceSize>(buff->length))};
        renderer->logicalDevice.updateDescriptorSets(
            WriteDescriptorSet(descriptorSet, idx, 0, descriptorSetLayoutBindings[idx].descriptorType, {}, c), nullptr);
    }
    catch (SystemError &e)
    {
        throw OMRendererException(VkErrorTranslate(e, "openminecraft.renderer.vk.err.pipeline.desc"));
    }
}
void OMRendererPipelineVk::bindInput(int idx, common::OMRendererTexture *texture)
{
    try
    {
        auto prop = renderer->physicalDevice.getProperties();
        auto fea = renderer->physicalDevice.getFeatures();

        auto textureSampler = renderer->logicalDevice.createSampler(
            SamplerCreateInfo({}, Filter::eLinear, Filter::eLinear, SamplerMipmapMode::eLinear,
                              SamplerAddressMode::eRepeat, SamplerAddressMode::eRepeat, SamplerAddressMode::eRepeat,
                              0.0f, fea.samplerAnisotropy, prop.limits.maxSamplerAnisotropy, false, CompareOp::eAlways,
                              0.0f, 0.0f, BorderColor::eIntOpaqueBlack, false),
            renderer->allocator);
        tempSamplers.push_back(textureSampler);

        auto cc = DescriptorImageInfo(textureSampler, reinterpret_cast<OMRendererTextureVk *>(texture)->imageView,
                                      ImageLayout::eShaderReadOnlyOptimal);
        renderer->logicalDevice.updateDescriptorSets(
            WriteDescriptorSet(descriptorSet, idx, 0, DescriptorType::eCombinedImageSampler, cc, {}), nullptr);
    }
    catch (SystemError &e)
    {
        throw OMRendererException(VkErrorTranslate(e, "openminecraft.renderer.vk.err.pipeline.desc"));
    }
}

static auto convert(common::OMRendererPipelineBlendType t) -> BlendFactor
{
    switch (t)
    {
    default:
    case common::One:
        return BlendFactor::eOne;
    case common::Zero:
        return BlendFactor::eZero;
    case common::Alpha:
        return BlendFactor::eSrcAlpha;
    case common::OneMinusAlpha:
        return BlendFactor::eOneMinusSrcAlpha;
    }
}

void OMRendererPipelineVk::build()
{
    if (available)
    {
        return;
    }

    for (auto id : shaderIds)
    {
        std::shared_ptr<common::OMShader> fin = nullptr;
        while (!fin)
        {
            fin = renderer->compiler.getResult(id);
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
        auto sm = renderer->logicalDevice.createShaderModule(
            ShaderModuleCreateInfo({}, fin->data.size(), reinterpret_cast<const uint32_t *>(fin->data.data())),
            renderer->allocator);
        shaders.push_back(sm);
        shaderCreateInfos.push_back({{}, convertTo(fin->typebase), sm, fin->entrypoint.c_str()});
    }
    shaderIds.clear();

    try
    {
        descriptorSetLayout = renderer->logicalDevice.createDescriptorSetLayout(
            DescriptorSetLayoutCreateInfo({}, descriptorSetLayoutBindings), renderer->allocator);

        if (descriptorSetLayoutBindings.size() > 0)
        {
            descriptorPool = renderer->logicalDevice.createDescriptorPool(
                DescriptorPoolCreateInfo(DescriptorPoolCreateFlagBits::eFreeDescriptorSet,
                                         descriptorSetLayoutBindings.size(), descriptorPoolSizes),
                renderer->allocator);
            descriptorSet =
                renderer->logicalDevice.allocateDescriptorSets({descriptorPool, 1, &descriptorSetLayout})[0];
        }

        pipelineLayout = renderer->logicalDevice.createPipelineLayout(PipelineLayoutCreateInfo({}, descriptorSetLayout),
                                                                      renderer->allocator);
    }
    catch (SystemError &e)
    {
        throw OMRendererException(VkErrorTranslate(e, "openminecraft.renderer.vk.err.pipeline.desc"));
    }

    auto vertexInput = PipelineVertexInputStateCreateInfo({}, vertexInputBindingDesc, vertexInputAttrDesc);
    auto inputAssembly = PipelineInputAssemblyStateCreateInfo({}, PrimitiveTopology::eTriangleList, false);
    auto rasterization = PipelineRasterizationStateCreateInfo(
        {}, false, false, PolygonMode::eFill, CullModeFlagBits::eNone, FrontFace::eCounterClockwise, true, 0, 0, 0, 1);
    auto multisample = PipelineMultisampleStateCreateInfo({}, SampleCountFlagBits::e1, false);
    auto viewportState = PipelineViewportStateCreateInfo({}, 1, nullptr, 1, nullptr);
    std::vector attc = {PipelineColorBlendAttachmentState(
        enableBlend, convert(blendState.srcColor), convert(blendState.dstColor), BlendOp::eAdd,
        convert(blendState.srcAlpha), convert(blendState.dstAlpha), BlendOp::eAdd,
        ColorComponentFlagBits::eA | ColorComponentFlagBits::eR | ColorComponentFlagBits::eG |
            ColorComponentFlagBits::eB)};
    auto colorblend =
        PipelineColorBlendStateCreateInfo({}, false, LogicOp::eNoOp, attc, std::array{0.f, 0.f, 0.f, 0.f});
    auto depthStencil = PipelineDepthStencilStateCreateInfo({}, enableDepthTest, enableDepthWrite,
                                                            enableReverseZ ? CompareOp::eGreater : CompareOp::eLess,
                                                            true, true, {}, {}, 0.0f, 1.0f);
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
        if (descriptorPool || descriptorSet)
        {
            renderer->logicalDevice.freeDescriptorSets(descriptorPool, descriptorSet);
            renderer->logicalDevice.destroyDescriptorPool(descriptorPool, renderer->allocator);
        }
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

void OMRendererPipelineVk::setBlendFunc(common::OMReedererPipelineBlendState state)
{
    this->blendState = state;
}

} // namespace openminecraft::renderer::vk
