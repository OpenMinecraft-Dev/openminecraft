#include "openminecraft/renderer/common/demiurge/element/om_demiurge_element_rect_channel.hpp"
#include "openminecraft/renderer/common/om_renderer_pipeline.hpp"
#include "openminecraft/renderer/common/om_renderer_shader.hpp"

namespace openminecraft::renderer::common::demiurge::element
{
void OMDemiurgeRectChannel::init(const std::string &uniformTag, const std::string &targetTag)
{
    const auto uniformBuffer = wrap::OMBufferRef(renderer, uniformTag);
    const auto target = wrap::OMRenderTargetRef(renderer, targetTag);

    format.setInstance()
        ->appendPart("inRectPos", basics::Vec4f)
        ->appendPart("inRectColor", basics::Vec4f)
        ->appendPart("inRectDepth", basics::Float)
        ->appendPart("inRectScale", basics::Vec2f)
        ->nextGroup()
        ->decideStruct();

    vtxShader = renderer->shaderManager.preprocess("demiurge/rect.vert.glsl", Vertex, GLSLSource, format);
    frgShader = renderer->shaderManager.preprocess("demiurge/rect.frag.glsl", Fragment, GLSLSource, format);

    OMDemiurgeQuadChannel::init(uniformTag, targetTag);
}
} // namespace openminecraft::renderer::common::demiurge::element
