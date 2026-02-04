#ifndef OM_RENDERER_LAYER_VK_PIPELINE_HPP
#define OM_RENDERER_LAYER_VK_PIPELINE_HPP

#include "openminecraft/renderer/common/basics/om_vertex_format.hpp"
#include "openminecraft/renderer/common/om_renderer_pipeline.hpp"
#include "openminecraft/renderer/common/om_renderer_shader.hpp"
#include "openminecraft/renderer/vk/om_renderer_layer_vk.hpp"
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
    void attachShader(std::shared_ptr<common::OMShader> shader) override;
    void vertexFormat(common::basics::OMVertexFormat format) override;
    void build() override;

    ::vk::Pipeline operator*()
    {
        return pipeline;
    };

  private:
    OMRendererVk *renderer;
    std::vector<::vk::ShaderModule> shaders;
    common::basics::OMVertexFormat format;

    ::vk::Pipeline pipeline;
};
} // namespace openminecraft::renderer::vk

#endif
