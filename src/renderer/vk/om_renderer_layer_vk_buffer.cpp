#include "openminecraft/renderer/vk/om_renderer_layer_vk_buffer.hpp"

using namespace vk;

namespace openminecraft::renderer::vk
{
OMRendererBufferVk::OMRendererBufferVk(common::OMBufferUsage usage, uint64_t length): OMRendererBuffer(usage, length)
{

}

static uint32_t findMemoryType(uint32_t typeFilter, MemoryPropertyFlags properties,
                               PhysicalDeviceMemoryProperties &memProperties)
{
    for (uint32_t i = 0; i < memProperties.memoryTypeCount; ++i)
    {
        if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties)
        {
            return i;
        }
    }

    return 0;
}

void OMRendererBufferVk::initialize()
{

}

void OMRendererBufferVk::release()
{

}

void OMRendererBufferVk::updateData(void *src)
{

}
}