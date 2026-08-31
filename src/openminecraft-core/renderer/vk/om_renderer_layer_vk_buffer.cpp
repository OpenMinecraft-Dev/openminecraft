#include "openminecraft/renderer/vk/om_renderer_layer_vk_buffer.hpp"

#include "openminecraft/i18n/om_i18n_res.hpp"
#include "openminecraft/renderer/common/om_renderer_buffer.hpp"
#include "openminecraft/renderer/om_renderer_exception.hpp"
#include "openminecraft/renderer/vk/om_renderer_layer_vk.hpp"
#include "vulkan/vulkan_enums.hpp"
#include "vulkan/vulkan_handles.hpp"
#include "vulkan/vulkan_structs.hpp"
#include <limits>

using namespace vk;
using namespace openminecraft::renderer::common;
using namespace openminecraft::i18n::res;

namespace openminecraft::renderer::vk
{
OMRendererBufferVk::OMRendererBufferVk(OMBufferUsage usage, uint64_t length, OMRendererVk *renderer)
    : OMRendererBuffer(usage, length, reinterpret_cast<OMRenderer *>(renderer))
{
    this->initialize();
}

OMRendererBufferVk::~OMRendererBufferVk()
{
    reinterpret_cast<OMRendererVk *>(renderer)->logicalDevice.waitIdle();
    this->release();
}

static auto findMemoryType(uint32_t typeFilter, MemoryPropertyFlags properties,
                           const PhysicalDeviceMemoryProperties &memProperties) -> uint32_t
{
    for (uint32_t i = 0; i < memProperties.memoryTypeCount; ++i)
    {
        if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties)
        {
            return i;
        }
    }

    throw OMRendererException(translate("openminecraft.renderer.vk.err.memory.type"));
}

static auto defFlags() -> MemoryPropertyFlags
{
    return MemoryPropertyFlagBits::eHostVisible | MemoryPropertyFlagBits::eHostCoherent;
}

static auto mapToUsageFlag(OMBufferUsage usage) -> BufferUsageFlagBits
{
    switch (usage)
    {
    case VertexData:
    case InstanceData:
    default:
        return BufferUsageFlagBits::eVertexBuffer;
    case VertexIndex:
        return BufferUsageFlagBits::eIndexBuffer;
    case Misc:
        return BufferUsageFlagBits::eTransferSrc;
    case Uniform:
        return BufferUsageFlagBits::eUniformBuffer;
    case Indirect:
        return BufferUsageFlagBits::eIndirectBuffer;
    case ShaderStorage:
        return BufferUsageFlagBits::eStorageBuffer;
    case UniformTexel:
        return BufferUsageFlagBits::eUniformTexelBuffer;
    }
}

void OMRendererBufferVk::initialize()
{
    try
    {
        auto renderer = reinterpret_cast<OMRendererVk *>(this->renderer);
        this->buffer = renderer->logicalDevice.createBuffer(
            BufferCreateInfo({}, this->length, mapToUsageFlag(this->usage), SharingMode::eExclusive),
            renderer->allocator);

        auto memprop = renderer->physicalDevice.getMemoryProperties();

        auto req = renderer->logicalDevice.getBufferMemoryRequirements(this->buffer);
        this->bufferMemory = renderer->logicalDevice.allocateMemory(
            MemoryAllocateInfo(req.size, findMemoryType(req.memoryTypeBits, defFlags(), memprop)), renderer->allocator);
        renderer->logicalDevice.bindBufferMemory(this->buffer, this->bufferMemory, 0);

        if (alwaysMapped)
        {
            data = renderer->logicalDevice.mapMemory(this->bufferMemory, 0, this->length);
        }
    }
    catch (SystemError &e)
    {
        throw OMRendererException(VkErrorTranslate(e, "openminecraft.renderer.vk.err.buffer"));
    }
}

void OMRendererBufferVk::release() const
{
    try
    {
        auto renderer = reinterpret_cast<OMRendererVk *>(this->renderer);

        if (alwaysMapped)
        {
            renderer->logicalDevice.unmapMemory(this->bufferMemory);
        }

        renderer->logicalDevice.destroyBuffer(this->buffer, renderer->allocator);
        renderer->logicalDevice.freeMemory(this->bufferMemory, renderer->allocator);
    }
    catch (SystemError &e)
    {
        throw OMRendererException(VkErrorTranslate(e, "openminecraft.renderer.vk.err.cleanup"));
    }
}

void OMRendererBufferVk::updateData(void *src)
{
    try
    {
        auto renderer = reinterpret_cast<OMRendererVk *>(this->renderer);

        if (alwaysMapped)
        {
            std::memcpy(data, src, this->length);
        }
        else
        {
            auto vtx = renderer->logicalDevice.mapMemory(this->bufferMemory, 0, this->length);
            if (vtx)
            {
                std::memcpy(vtx, src, this->length);
            }
            renderer->logicalDevice.unmapMemory(this->bufferMemory);
        }
    }
    catch (SystemError &e)
    {
        throw OMRendererException(VkErrorTranslate(e, "openminecraft.renderer.vk.err.buffer.update"));
    }
}

void OMRendererBufferVk::updateDataPart(void *src, uint64_t offset, uint64_t length)
{
    try
    {
        auto renderer = reinterpret_cast<OMRendererVk *>(this->renderer);
        if (renderer->currentFence != Fence{})
        {
            (void)renderer->logicalDevice.waitForFences(renderer->currentFence, true,
                                                        std::numeric_limits<uint64_t>::max());
        }
        if (alwaysMapped)
        {
            std::memcpy(reinterpret_cast<void *>(reinterpret_cast<uintptr_t>(data) + offset), src, length);
        }
        else
        {
            auto vtx = renderer->logicalDevice.mapMemory(this->bufferMemory, 0, this->length);
            std::memcpy(reinterpret_cast<void *>(reinterpret_cast<uintptr_t>(vtx) + offset), src, length);
            renderer->logicalDevice.unmapMemory(this->bufferMemory);
        }
    }
    catch (SystemError &e)
    {
        throw OMRendererException(VkErrorTranslate(e, "openminecraft.renderer.vk.err.buffer.update"));
    }
}

void OMRendererBufferVk::copyTo(OMRendererBuffer *dst)
{
    try
    {
        auto renderer = reinterpret_cast<OMRendererVk *>(this->renderer);
        if (renderer->currentFence != Fence{})
        {
            (void)renderer->logicalDevice.waitForFences(renderer->currentFence, true,
                                                        std::numeric_limits<uint64_t>::max());
        }
        if (alwaysMapped)
        {
            dst->updateDataPart(data, 0, length);
        }
        else
        {
            auto vtx = renderer->logicalDevice.mapMemory(this->bufferMemory, 0, this->length);
            dst->updateDataPart(vtx, 0, length);
            renderer->logicalDevice.unmapMemory(this->bufferMemory);
        }
    }
    catch (SystemError &e)
    {
        throw OMRendererException(VkErrorTranslate(e, "openminecraft.renderer.vk.err.buffer.update"));
    }
}
} // namespace openminecraft::renderer::vk
