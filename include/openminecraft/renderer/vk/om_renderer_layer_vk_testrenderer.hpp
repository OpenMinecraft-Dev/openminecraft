#ifndef OM_RENDERER_LAYER_VK_TESTRENDERER
#define OM_RENDERER_LAYER_VK_TESTRENDERER
#include "openminecraft/util/om_util_reinitable.hpp"
#ifdef OM_VULKAN_DYNAMIC
#define VULKAN_HPP_DISPATCH_LOADER_DYNAMIC 1
#endif
#include "openminecraft/renderer/common/om_renderer_shader.hpp"
#include "vulkan/vulkan.hpp"

#include <memory>

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
    std::vector<::vk::Framebuffer> framebuffers;
    ::vk::PipelineLayout pipelineLayout;
    ::vk::Pipeline pipeline;
    ::vk::Buffer vertexBuffer;
    ::vk::DeviceMemory vertexBufferMemory;
    ::vk::Buffer indexBuffer;
    ::vk::DeviceMemory indexBufferMemory;
    ::vk::CommandPool commandPool;
    std::vector<::vk::CommandBuffer> commandBuffers;
private:
    bool firstTime = true;
    OMRendererVk *renderer;

    std::shared_ptr<common::OMShader> vtxShader;
    std::shared_ptr<common::OMShader> frgShader;
};
}

#endif