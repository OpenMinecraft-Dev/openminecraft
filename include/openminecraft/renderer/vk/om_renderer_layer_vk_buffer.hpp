#ifndef OM_RENDERER_LAYER_VK_BUFFER_HPP
#define OM_RENDERER_LAYER_VK_BUFFER_HPP
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
    void moveDataPart(uint64_t target, uint64_t original, uint64_t length) override;
    void copyTo(OMRendererBuffer *) override;
    auto fetchData() -> std::shared_ptr<void> override;

    ::vk::Buffer buffer;
    ::vk::DeviceMemory bufferMemory;

  private:
    void initialize();
    void release() const;
};
} // namespace openminecraft::renderer::vk

#endif
