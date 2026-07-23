#include "openminecraft/renderer/vk/om_renderer_layer_vk_buffer.hpp"

#include "openminecraft/i18n/om_i18n_res.hpp"
#include "openminecraft/renderer/common/om_renderer_buffer.hpp"
#include "openminecraft/renderer/om_renderer_exception.hpp"
#include "openminecraft/renderer/vk/om_renderer_layer_vk.hpp"
#include "vulkan/vulkan_enums.hpp"

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
    ;
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

        renderer->logicalDevice.freeMemory(this->bufferMemory, renderer->allocator);
        renderer->logicalDevice.destroyBuffer(this->buffer, renderer->allocator);
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
        auto vtx = renderer->logicalDevice.mapMemory(this->bufferMemory, 0, this->length);
        std::memcpy(vtx, src, this->length);

        if (alwaysMapped)
        {
            return;
        }

        renderer->logicalDevice.unmapMemory(this->bufferMemory);
    }
    catch (SystemError &e)
    {
        throw OMRendererException(VkErrorTranslate(e, "openminecraft.renderer.vk.err.buffer.update"));
    }
}
} // namespace openminecraft::renderer::vk
