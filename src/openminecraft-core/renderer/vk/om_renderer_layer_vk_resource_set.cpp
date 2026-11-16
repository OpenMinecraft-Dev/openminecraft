#include "openminecraft/renderer/vk/om_renderer_layer_vk_resource_set.hpp"
#include "openminecraft/renderer/vk/om_renderer_layer_vk.hpp"
#include "openminecraft/renderer/vk/om_renderer_layer_vk_buffer.hpp"
#include "openminecraft/renderer/vk/om_renderer_layer_vk_pipeline.hpp"
#include "openminecraft/renderer/vk/om_renderer_layer_vk_texture.hpp"
#include "openminecraft/renderer/om_renderer_exception.hpp"
#include "openminecraft/i18n/om_i18n_res.hpp"

using namespace vk;
using namespace openminecraft::renderer::common;
using namespace openminecraft::i18n::res;

namespace openminecraft::renderer::vk
{
OMRendererResourceSetVk::OMRendererResourceSetVk(OMRendererVk *renderer, OMRendererPipelineVk *pipeline)
    : OMRendererResourceSet(renderer, pipeline), vkRenderer(renderer), vkPipeline(pipeline)
{
    const auto &bindings = pipeline->getDescriptorLayoutBindings();
    if (bindings.empty())
    {
        return;
    }

    try
    {
        pool = renderer->logicalDevice.createDescriptorPool(
            DescriptorPoolCreateInfo(DescriptorPoolCreateFlagBits::eFreeDescriptorSet, bindings.size(),
                                     pipeline->getDescriptorPoolSizes()),
            renderer->allocator);
        // The pipeline is a friend, so the resource set can use the shared layout.
        set = renderer->logicalDevice.allocateDescriptorSets({pool, 1, &pipeline->descriptorSetLayout})[0];
    }
    catch (SystemError &e)
    {
        throw OMRendererException(VkErrorTranslate(e, "openminecraft.renderer.vk.err.pipeline.desc"));
    }
}

OMRendererResourceSetVk::~OMRendererResourceSetVk()
{
    if (vkRenderer == nullptr)
    {
        return;
    }

    for (auto &p : bufferViews)
    {
        vkRenderer->logicalDevice.destroyBufferView(p.second, vkRenderer->allocator);
    }
    bufferViews.clear();

    if (pool || set)
    {
        if (set)
        {
            vkRenderer->logicalDevice.freeDescriptorSets(pool, set);
        }
        vkRenderer->logicalDevice.destroyDescriptorPool(pool, vkRenderer->allocator);
    }
}

void OMRendererResourceSetVk::bindInput(int index, OMRendererBuffer *buffer)
{
    if (!set)
    {
        return;
    }

    const auto &bindings = vkPipeline->getDescriptorLayoutBindings();
    if (index < 0 || static_cast<size_t>(index) >= bindings.size())
    {
        return;
    }

    try
    {
        if (buffer->usage == UniformTexel)
        {
            auto bv = vkRenderer->logicalDevice.createBufferView(
                BufferViewCreateInfo({}, reinterpret_cast<OMRendererBufferVk *>(buffer)->buffer,
                                     OMRendererPipelineVk::convertTo(buffer->format), 0,
                                     static_cast<DeviceSize>(buffer->length)),
                vkRenderer->allocator);
            vkRenderer->logicalDevice.updateDescriptorSets(
                WriteDescriptorSet(set, index, 0, bindings[index].descriptorType, {}, {}, {bv}), nullptr);

            auto previous = bufferViews.find(static_cast<uint32_t>(index));
            if (previous != bufferViews.end())
            {
                vkRenderer->logicalDevice.destroyBufferView(previous->second, vkRenderer->allocator);
                previous->second = bv;
            }
            else
            {
                bufferViews.emplace(static_cast<uint32_t>(index), bv);
            }
            return;
        }

        std::vector<DescriptorBufferInfo> info = {DescriptorBufferInfo(
            reinterpret_cast<OMRendererBufferVk *>(buffer)->buffer, 0, static_cast<DeviceSize>(buffer->length))};
        vkRenderer->logicalDevice.updateDescriptorSets(
            WriteDescriptorSet(set, index, 0, bindings[index].descriptorType, {}, info), nullptr);
    }
    catch (SystemError &e)
    {
        throw OMRendererException(VkErrorTranslate(e, "openminecraft.renderer.vk.err.pipeline.desc"));
    }
}

void OMRendererResourceSetVk::bindInput(int index, OMRendererTexture *texture)
{
    if (!set)
    {
        return;
    }

    const auto &bindings = vkPipeline->getDescriptorLayoutBindings();
    if (index < 0 || static_cast<size_t>(index) >= bindings.size())
    {
        return;
    }

    try
    {
        auto imageInfo = DescriptorImageInfo(reinterpret_cast<OMRendererTextureVk *>(texture)->sampler,
                                             reinterpret_cast<OMRendererTextureVk *>(texture)->imageView,
                                             ImageLayout::eShaderReadOnlyOptimal);
        vkRenderer->logicalDevice.updateDescriptorSets(
            WriteDescriptorSet(set, index, 0, DescriptorType::eCombinedImageSampler, imageInfo, {}), nullptr);
    }
    catch (SystemError &e)
    {
        throw OMRendererException(VkErrorTranslate(e, "openminecraft.renderer.vk.err.pipeline.desc"));
    }
}
} // namespace openminecraft::renderer::vk
