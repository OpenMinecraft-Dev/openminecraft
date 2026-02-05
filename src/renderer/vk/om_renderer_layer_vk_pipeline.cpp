#include "openminecraft/renderer/vk/om_renderer_layer_vk_pipeline.hpp"
#include "openminecraft/i18n/om_i18n_res.hpp"
#include "openminecraft/renderer/common/basics/om_vertex_format.hpp"
#include "openminecraft/renderer/common/om_renderer_pipeline.hpp"
#include "openminecraft/renderer/common/om_renderer_shader.hpp"
#include "openminecraft/renderer/vk/om_renderer_layer_vk.hpp"
#include "vulkan/vulkan.hpp"
#include <stdexcept>

using openminecraft::i18n::res::translate;
using namespace ::vk;

namespace openminecraft::renderer::vk
{
OMRendererPipelineVk::OMRendererPipelineVk(OMRendererVk *renderer)
    : renderer(renderer), common::OMRendererPipeline(renderer)
{
}

OMRendererPipelineVk::~OMRendererPipelineVk()
{
}

void OMRendererPipelineVk::attachShader(std::shared_ptr<common::OMShader> shader)
{
    auto fin = shader;
    if (fin->type != common::OMShaderFileType::SPIRVBinary)
    {
        fin = fin->convertTo(common::OMShaderFileType::GLSLSource);
    }
    if (fin == nullptr)
    {
        throw std::logic_error(translate("openminecraft.renderer.vk.err.shaderstat"));
    }
    shaders.push_back(renderer->logicalDevice.createShaderModule(ShaderModuleCreateInfo(
        {}, shader->data.size(), reinterpret_cast<const uint32_t *>(shader->data.data()), renderer->allocator)));
}

void OMRendererPipelineVk::vertexFormat(common::basics::OMVertexFormat format)
{
    this->format = format;
}

void OMRendererPipelineVk::build()
{
}

} // namespace openminecraft::renderer::vk
