#ifndef OM_RENDERER_LAYER_VK_BUFFER
#define OM_RENDERER_LAYER_VK_BUFFER
#include "openminecraft/renderer/common/om_renderer_buffer.hpp"

#ifdef OM_VULKAN_DYNAMIC
#define VULKAN_HPP_DISPATCH_LOADER_DYNAMIC 1
#endif
#include "vulkan/vulkan.hpp"
#include "vulkan/vulkan_core.h"

namespace openminecraft::renderer::vk
{
class OMRendererBufferVk: public common::OMRendererBuffer
{
  public:
    OMRendererBufferVk(common::OMBufferUsage usage, uint64_t length);

    void initialize() override;
    void release() override;

    void updateData(void *src) override;

private:
    ::vk::Buffer buffer;
    ::vk::DeviceMemory bufferMemory;
};
}

#endif