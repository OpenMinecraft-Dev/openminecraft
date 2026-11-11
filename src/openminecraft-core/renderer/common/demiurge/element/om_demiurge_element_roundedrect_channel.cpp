#include "openminecraft/renderer/common/demiurge/element/om_demiurge_element_roundedrect_channel.hpp"
#include "openminecraft/renderer/common/basics/om_vertex_format.hpp"
#include "openminecraft/renderer/common/demiurge/element/om_demiurge_element_quad_channel.hpp"
#include "openminecraft/renderer/common/om_renderer_pipeline.hpp"
#include "openminecraft/renderer/common/om_renderer_shader.hpp"

namespace openminecraft::renderer::common::demiurge::element
{
void OMDemiurgeRoundedRectChannel::init(const std::string &uniformTag, const std::string &targetTag)
{
    const auto uniformBuffer = wrap::OMBufferRef(renderer, uniformTag);
    const auto target = wrap::OMRenderTargetRef(renderer, targetTag);

    format.setInstance()
        ->appendPart("inRectPos", basics::Vec4f)
        ->appendPart("inRectColor", basics::Vec4f)
        ->appendPart("inRectRadius", basics::Vec4f)
        ->appendPart("inRectRotation", basics::Vec4f)
        ->appendPart("inRectFactor", basics::Float)
        ->appendPart("inRectDepth", basics::UInteger)
        ->appendPart("inRectScale", basics::Vec2f)
        ->appendPart("inClipPos", basics::Vec4f)
        ->appendPart("inClipRadius", basics::Float)
        ->nextGroup()
        ->decideStruct();

    vtxShader = renderer->shaderManager.preprocess("demiurge/roundedrect.vert.glsl", Vertex, GLSLSource, format);
    frgShader = renderer->shaderManager.preprocess("demiurge/roundedrect.frag.glsl", Fragment, GLSLSource, format);

    OMDemiurgeQuadChannel::init(uniformTag, targetTag);
}
} // namespace openminecraft::renderer::common::demiurge::element
