#include "openminecraft/renderer/vk/om_renderer_layer_vk_buffer.hpp"

#include "openminecraft/renderer/vk/om_renderer_layer_vk.hpp"
#include <stdexcept>

using namespace vk;
using namespace openminecraft::renderer::common;

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

static uint32_t findMemoryType(uint32_t typeFilter, MemoryPropertyFlags properties,
                               const PhysicalDeviceMemoryProperties &memProperties)
{
    for (uint32_t i = 0; i < memProperties.memoryTypeCount; ++i)
    {
        if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties)
        {
            return i;
        }
    }

    throw std::runtime_error("memory type not found!");
}

static MemoryPropertyFlags defFlags()
{
    return MemoryPropertyFlagBits::eHostVisible | MemoryPropertyFlagBits::eHostCoherent;
    ;
}

static BufferUsageFlagBits mapToUsageFlag(OMBufferUsage usage)
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
    }
}

void OMRendererBufferVk::initialize()
{
    auto renderer = reinterpret_cast<OMRendererVk *>(this->renderer);
    this->buffer = renderer->logicalDevice.createBuffer(
        BufferCreateInfo({}, this->length, mapToUsageFlag(this->usage), SharingMode::eExclusive), renderer->allocator);

    auto memprop = renderer->physicalDevice.getMemoryProperties();

    auto req = renderer->logicalDevice.getBufferMemoryRequirements(this->buffer);
    this->bufferMemory = renderer->logicalDevice.allocateMemory(
        MemoryAllocateInfo(req.size, findMemoryType(req.memoryTypeBits, defFlags(), memprop)), renderer->allocator);
    renderer->logicalDevice.bindBufferMemory(this->buffer, this->bufferMemory, 0);
}

void OMRendererBufferVk::release() const
{
    auto renderer = reinterpret_cast<OMRendererVk *>(this->renderer);

    if (alwaysMapped)
    {
        renderer->logicalDevice.unmapMemory(this->bufferMemory);
    }

    renderer->logicalDevice.freeMemory(this->bufferMemory, renderer->allocator);
    renderer->logicalDevice.destroyBuffer(this->buffer, renderer->allocator);
}

void OMRendererBufferVk::updateData(void *src)
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
} // namespace openminecraft::renderer::vk
