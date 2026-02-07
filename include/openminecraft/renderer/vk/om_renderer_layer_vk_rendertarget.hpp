#ifndef OM_RENDERER_LAYER_VK_RENDERTARGET_HPP
#define OM_RENDERER_LAYER_VK_RENDERTARGET_HPP

#include "openminecraft/renderer/common/om_renderer_rendertarget.hpp"
#include "openminecraft/renderer/common/om_renderer_texture.hpp"
#include "openminecraft/renderer/vk/om_renderer_layer_vk.hpp"

#ifdef OM_VULKAN_DYNAMIC
#define VULKAN_HPP_DISPATCH_LOADER_DYNAMIC 1
#endif

#include "vulkan/vulkan.hpp"

namespace openminecraft::renderer::vk
{
struct OMRendererRenderTargetBlock
{
    ::vk::Framebuffer framebuffer;
};

class OMRendererRenderTargetVk : public common::OMRendererRenderTarget
{
  public:
    OMRendererRenderTargetVk(OMRendererVk *renderer);
    ~OMRendererRenderTargetVk() override;

    void attachTarget(common::OMRendererTexture *texture) override;
    glm::vec2 fetchSize() override;
    void build() override;

    ::vk::RenderPass renderPass;

    OMRendererRenderTargetBlock *block = nullptr;

  private:
    OMRendererVk *renderer;
    std::vector<common::OMRendererTexture *> textures;
    bool available = false;
};
} // namespace openminecraft::renderer::vk

#endif
