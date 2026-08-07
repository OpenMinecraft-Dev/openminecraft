#ifndef OM_RENDERER_LAYER_VK_TEXTURE_HPP
#define OM_RENDERER_LAYER_VK_TEXTURE_HPP
#include "om_renderer_layer_vk.hpp"
#include "openminecraft/renderer/common/om_renderer_texture.hpp"

#ifdef OM_VULKAN_DYNAMIC
#define VULKAN_HPP_DISPATCH_LOADER_DYNAMIC 1
#endif
#include "vulkan/vulkan.hpp"
#include "vulkan/vulkan_core.h"

namespace openminecraft::renderer::vk
{
class OMRendererTextureVk : public common::OMRendererTexture
{
  public:
    OMRendererTextureVk(uint64_t width, uint64_t height, common::OMTextureType type, common::OMTextureArrangement arr,
                        OMRendererVk *renderer);
    ~OMRendererTextureVk() override;

    void transitionImageLayout(::vk::CommandBuffer cmd, ::vk::ImageLayout oldLayout, ::vk::ImageLayout newLayout);
    void updateData(void *p) override;
    void updateDataPart(void *p, uint64_t x, uint64_t y, uint64_t w, uint64_t h) override;

    ::vk::Format format;
    ::vk::Image image;
    ::vk::ImageView imageView;
    ::vk::DeviceMemory imageMemory;

  private:
    OMRendererVk *renderer;
};
} // namespace openminecraft::renderer::vk

#endif
