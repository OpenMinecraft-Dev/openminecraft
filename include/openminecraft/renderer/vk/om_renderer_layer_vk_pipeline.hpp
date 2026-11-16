#ifndef OM_RENDERER_LAYER_VK_PIPELINE_HPP
#define OM_RENDERER_LAYER_VK_PIPELINE_HPP

#include "openminecraft/log/om_log_common.hpp"
#include "openminecraft/renderer/common/basics/om_vertex_format.hpp"
#include "openminecraft/renderer/common/om_renderer_pipeline.hpp"
#include "openminecraft/renderer/common/om_renderer_rendertarget.hpp"
#include "openminecraft/renderer/common/om_renderer_shader.hpp"
#include "openminecraft/renderer/common/om_renderer_texture.hpp"
#include "openminecraft/renderer/vk/om_renderer_layer_vk.hpp"
#include <memory>
#include <unordered_map>
#include <vector>

#ifdef OM_VULKAN_DYNAMIC
#define VULKAN_HPP_DISPATCH_LOADER_DYNAMIC 1
#endif

#include "vulkan/vulkan.hpp"

namespace openminecraft::renderer::vk
{
class OMRendererPipelineVk : public common::OMRendererPipeline
{
  public:
    OMRendererPipelineVk(OMRendererVk *renderer);
    ~OMRendererPipelineVk() override;
    void appendInput(common::OMRendererPipelineInputType type) override;
    void attachShader(std::shared_ptr<common::OMShader> shader) override;
    void vertexFormat(common::basics::OMVertexFormat format) override;
    void bindOutput(common::OMRendererRenderTarget *target) override;

    void bindInput(int idx, common::OMRendererBuffer *buff) override;
    void bindInput(int idx, common::OMRendererTexture *texture) override;
    void bindInputName(std::string name) override
    {
    }

    void build() override;
    void setBlendFunc(common::OMReedererPipelineBlendState state) override;
    static auto convertTo(common::basics::OMVertexPropType) -> ::vk::Format;
    static auto convertTo(common::OMShaderType) -> ::vk::ShaderStageFlagBits;

    auto getPipeline() -> ::vk::Pipeline
    {
        return pipeline;
    };

    auto getPipelineLayout() -> ::vk::PipelineLayout
    {
        return pipelineLayout;
    }

    auto getDescriptorLayoutBindings() const -> const std::vector<::vk::DescriptorSetLayoutBinding> &
    {
        return descriptorSetLayoutBindings;
    }

    auto getDescriptorPoolSizes() const -> const std::vector<::vk::DescriptorPoolSize> &
    {
        return descriptorPoolSizes;
    }

    auto createResourceSet() -> common::OMRendererResourceSet * override;

  private:
    friend class OMRendererResourceSetVk;
    OMRendererVk *renderer;
    std::vector<::vk::ShaderModule> shaders;
    std::vector<::vk::PipelineShaderStageCreateInfo> shaderCreateInfos;
    std::vector<::vk::VertexInputBindingDescription> vertexInputBindingDesc;
    std::vector<::vk::VertexInputAttributeDescription> vertexInputAttrDesc;
    std::vector<::vk::DescriptorSetLayoutBinding> descriptorSetLayoutBindings;
    std::vector<::vk::DescriptorPoolSize> descriptorPoolSizes;
    int layoutBinding = 0;
    common::OMRendererRenderTarget *target;

    ::vk::Pipeline pipeline;
    ::vk::PipelineLayout pipelineLayout;
    ::vk::DescriptorSetLayout descriptorSetLayout;
    bool available = false;

    log::OMLogger logger;

    std::vector<int> shaderIds;

    std::vector<common::OMReedererPipelineBlendState> blendState;
};
} // namespace openminecraft::renderer::vk

#endif
