#ifndef OM_RENDERER_LAYER_VK_RESOURCE_SET_HPP
#define OM_RENDERER_LAYER_VK_RESOURCE_SET_HPP

#include "openminecraft/renderer/common/om_renderer_resource_set.hpp"
#ifdef OM_VULKAN_DYNAMIC
#define VULKAN_HPP_DISPATCH_LOADER_DYNAMIC 1
#endif
#include "vulkan/vulkan.hpp"
#include <unordered_map>
#include <vector>

namespace openminecraft::renderer::vk
{
class OMRendererVk;
class OMRendererPipelineVk;

class OMRendererResourceSetVk : public common::OMRendererResourceSet
{
  public:
    OMRendererResourceSetVk(OMRendererVk *renderer, OMRendererPipelineVk *pipeline);
    ~OMRendererResourceSetVk() override;

    void bindInput(int index, common::OMRendererBuffer *buffer) override;
    void bindInput(int index, common::OMRendererTexture *texture) override;
    void bindInputName(std::string name) override
    {
    }

    auto descriptorSet() const -> ::vk::DescriptorSet
    {
        return set;
    }

  private:
    OMRendererVk *vkRenderer;
    OMRendererPipelineVk *vkPipeline;
    ::vk::DescriptorPool pool;
    ::vk::DescriptorSet set;
    std::unordered_map<uint32_t, ::vk::BufferView> bufferViews;
};
} // namespace openminecraft::renderer::vk

#endif
