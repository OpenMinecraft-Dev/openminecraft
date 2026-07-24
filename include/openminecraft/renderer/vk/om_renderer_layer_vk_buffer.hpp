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
class OMRendererVk;
class OMRendererBufferVk : public common::OMRendererBuffer
{
  public:
    OMRendererBufferVk(common::OMBufferUsage usage, uint64_t length, OMRendererVk *renderer);
    ~OMRendererBufferVk() override;

    void updateData(void *src) override;
    void updateDataPart(void *src, uint64_t offset, uint64_t length) override;

    ::vk::Buffer buffer;
    ::vk::DeviceMemory bufferMemory;

  private:
    void initialize();
    void release() const;
};
} // namespace openminecraft::renderer::vk

#endif
