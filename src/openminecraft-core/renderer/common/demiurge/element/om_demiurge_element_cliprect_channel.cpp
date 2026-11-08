#include "openminecraft/renderer/common/demiurge/element/om_demiurge_element_cliprect_channel.hpp"
#include "openminecraft/renderer/common/om_renderer_pipeline.hpp"
#include "openminecraft/renderer/common/om_renderer_shader.hpp"

namespace openminecraft::renderer::common::demiurge::element
{
void OMDemiurgeClipRectChannel::init(const std::string &uniformTag, const std::string &targetTag)
{
    const auto uniformBuffer = wrap::OMBufferRef(renderer, uniformTag);
    const auto target = wrap::OMRenderTargetRef(renderer, targetTag);

    format.setInstance()
        ->appendPart("inRectPos", basics::Vec4f)
        ->appendPart("inRectDepth", basics::Float)
        ->nextGroup()
        ->decideStruct();

    vtxShader = renderer->shaderManager.preprocess("demiurge/cliprect.vert.glsl", Vertex, GLSLSource, format);
    frgShader = renderer->shaderManager.preprocess("demiurge/cliprect.frag.glsl", Fragment, GLSLSource, format);

    initPipeline(uniformBuffer, target, false);
}
} // namespace openminecraft::renderer::common::demiurge::element
