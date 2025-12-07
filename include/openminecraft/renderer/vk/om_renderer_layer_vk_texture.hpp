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
class OMRendererTextureVk : common::OMRendererTexture
{
  public:
    OMRendererTextureVk(uint64_t width, uint64_t height, common::OMTextureType type, common::OMTextureArrangement arr, OMRendererVk *renderer);
    ~OMRendererTextureVk() override;

  private:
    ::vk::Image image;
    ::vk::ImageView imageView;
    ::vk::DeviceMemory imageMemory;
};
} // namespace openminecraft::renderer::vk

#endif