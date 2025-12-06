#ifndef OM_RENDERER_LAYER_VK_TESTRENDERER
#define OM_RENDERER_LAYER_VK_TESTRENDERER
#include "openminecraft/log/om_log_common.hpp"
#include "openminecraft/util/om_util_reinitable.hpp"
#ifdef OM_VULKAN_DYNAMIC
#define VULKAN_HPP_DISPATCH_LOADER_DYNAMIC 1
#endif
#include "openminecraft/renderer/common/om_renderer_shader.hpp"
#include "vulkan/vulkan.hpp"

#include <chrono>

#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_FORCE_RADIANS
#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include <memory>

namespace openminecraft::renderer::common
{
class OMRendererBuffer;
}
namespace openminecraft::renderer::vk
{
class OMRendererVk;
}

namespace openminecraft::renderer::vk::test
{
struct UniformStructure
{
    glm::mat4 model;
    glm::mat4 view;
    glm::mat4 proj;
};

class OMTestRenderer : public util::OMReinitable
{
  public:
    OMTestRenderer(OMRendererVk *renderer);
    ~OMTestRenderer() = default;

    ::vk::CommandBuffer beginSingleTimeCommands();
    void endSingleTimeCommands(::vk::CommandBuffer cmdbuff);
    void copyBuffer(::vk::Buffer srcBuff, ::vk::Buffer dstBuff, ::vk::DeviceSize size);
    void transitionImageLayout(::vk::Image image, ::vk::Format format, ::vk::ImageLayout oldLayout,
                               ::vk::ImageLayout newLayout);
    void copyBufferToImage(::vk::Buffer buffer, ::vk::Image image, uint32_t width, uint32_t height);

    void reinit() override;
    void destroy();

    void updateUniform();

    ::vk::RenderPass renderPass;
    std::vector<::vk::Framebuffer> framebuffers;
    ::vk::PipelineLayout pipelineLayout;
    ::vk::Pipeline pipeline;

    common::OMRendererBuffer *vertexBuffer;
    common::OMRendererBuffer *indexBuffer;

    ::vk::CommandPool commandPool;
    std::vector<::vk::CommandBuffer> commandBuffers;

    std::vector<::vk::DescriptorSetLayout> descriptorSetLayouts;

    common::OMRendererBuffer *uniformBuffer;

    ::vk::DescriptorPool descriptorPool;
    ::vk::DescriptorSet descriptorSet;
    ::vk::DescriptorSet combinedDescriptorSet;

    ::vk::Buffer stagingBuffer;
    ::vk::DeviceMemory stagingBufferMemory;

    ::vk::DeviceMemory imageMemory;
    ::vk::Image textureImage;
    ::vk::ImageView textureImageView;

    ::vk::Sampler textureSampler;

    ::vk::DeviceMemory depthImageMemory;
    ::vk::Image depthImage;
    ::vk::ImageView depthImageView;

    void keyInput(bool w, bool a, bool s, bool d, bool lsh, bool sp, bool upk, bool downk, bool leftk, bool rightk);

    log::OMLogger logger;

  private:
    bool firstTime = true;
    int vertexCount = 0;
    OMRendererVk *renderer;

    glm::vec3 m_cameraPos{2.0f, 2.0f, 2.0f};
    glm::vec3 m_cameraUp{0.0f, 1.0f, 0.0f};
    float m_pitch = -35.0f;
    float m_yaw = -135.0f;
    float m_cameraMoveSpeed = 1.0f;
    float m_cameraRotateSpeed = 45.0f;

    std::shared_ptr<common::OMShader> vtxShader;
    std::shared_ptr<common::OMShader> frgShader;
};
} // namespace openminecraft::renderer::vk::test

#endif
