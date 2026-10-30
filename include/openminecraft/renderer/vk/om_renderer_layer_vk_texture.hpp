#ifndef OM_RENDERER_LAYER_VK_TEXTURE_HPP
#define OM_RENDERER_LAYER_VK_TEXTURE_HPP
#include "om_renderer_layer_vk.hpp"
#include "openminecraft/renderer/common/om_renderer_buffer.hpp"
#include "openminecraft/renderer/common/om_renderer_texture.hpp"
#include "vulkan/vulkan_enums.hpp"
#include "vulkan/vulkan_handles.hpp"
#include <cstdint>
#include <unordered_map>

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
    OMRendererTextureVk(uint64_t width, uint64_t height, uint64_t layers, uint64_t mipmap, common::OMTextureType type,
                        common::OMTextureArrangement arr, OMRendererVk *renderer);
    ~OMRendererTextureVk() override;

    void transitionImageLayout(::vk::CommandBuffer cmd, ::vk::ImageLayout oldLayout, ::vk::ImageLayout newLayout,
                               uint64_t mip, uint64_t baseLayer, uint64_t layers,
                               ::vk::ImageAspectFlags aspect = ::vk::ImageAspectFlagBits::eColor);
    void updateData(void *p, uint64_t) override;
    void updateDataPart(void *p, uint64_t x, uint64_t y, uint64_t w, uint64_t h, uint64_t) override;
    void updateDataMulti(const std::vector<common::OMRendererTexture::OMTextureDataUpdate> &updates) override;
    void setupSampler() override;
    auto fetchData(uint64_t layer = 0) -> std::shared_ptr<void> override;

    ::vk::Format format;
    ::vk::Image image;
    ::vk::ImageView imageView;
    ::vk::DeviceMemory imageMemory;
    ::vk::Sampler sampler;

    ::vk::SampleCountFlagBits sampleCount;

  private:
    struct OMTextureUpdateSlot
    {
        ::vk::CommandBuffer cmd = nullptr;
        ::vk::Fence fence = nullptr;
        common::OMRendererBuffer *staging = nullptr;
        bool inFlight = false;
        uint64_t capacity = 0;
    };

    auto acquireUpdateSlot(uint64_t bytes) -> OMTextureUpdateSlot &;

    uint64_t mipmap;
    uint64_t layers;
    OMRendererVk *renderer;
    std::unordered_map<uint64_t, bool> uploaded;

    OMTextureUpdateSlot updateSlot = {};
};
} // namespace openminecraft::renderer::vk

#endif
