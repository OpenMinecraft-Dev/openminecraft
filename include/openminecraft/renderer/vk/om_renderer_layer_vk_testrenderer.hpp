#ifndef OM_RENDERER_LAYER_VK_TESTRENDERER
#define OM_RENDERER_LAYER_VK_TESTRENDERER
#include "openminecraft/util/om_util_reinitable.hpp"
#ifdef OM_VULKAN_DYNAMIC
#define VULKAN_HPP_DISPATCH_LOADER_DYNAMIC 1
#endif
#include "vulkan/vulkan.hpp"

namespace openminecraft::renderer::vk
{
class OMRendererVk;
}

namespace openminecraft::renderer::vk::test
{
class OMTestRenderer : public util::OMReinitable
{
public:
    OMTestRenderer(OMRendererVk *renderer);
    ~OMTestRenderer() = default;

    void reinit() override;
    void destroy();

    ::vk::RenderPass renderPass;
private:
    OMRendererVk *renderer;
};
}

#endif