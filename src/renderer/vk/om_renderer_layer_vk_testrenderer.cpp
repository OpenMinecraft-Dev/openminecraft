#include "openminecraft/renderer/vk/om_renderer_layer_vk_testrenderer.hpp"

#include "openminecraft/renderer/vk/om_renderer_layer_vk.hpp"
#include "openminecraft/renderer/vk/om_renderer_layer_vk_validation.hpp"

#include <vulkan/vulkan_core.h>

using namespace ::vk;

namespace openminecraft::renderer::vk::test
{
OMTestRenderer::OMTestRenderer(OMRendererVk *renderer): renderer(renderer)
{
    OMTestRenderer::reinit();
}

void OMTestRenderer::reinit()
{
    /*auto result = renderer->logicalDevice.createRenderPass(RenderPassCreateInfo(), renderer->allocator);
    (void)result;*/
}
void OMTestRenderer::destroy()
{

}

}