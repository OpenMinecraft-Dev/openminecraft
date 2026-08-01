#include "openminecraft/renderer/common/demiurge/element/om_demiurge_element_rect_channel.hpp"
#include "openminecraft/renderer/common/om_renderer_pipeline.hpp"
#include "openminecraft/renderer/common/om_renderer_shader.hpp"

namespace openminecraft::renderer::common::demiurge::element
{
void OMDemiurgeRectChannel::init(OMRendererBuffer *uniformBuffer, OMRendererRenderTarget *target)
{
    vtxShader = renderer->shaderManager.preprocess("demiurge/rect.vert.glsl", Vertex, GLSLSource);
    frgShader = renderer->shaderManager.preprocess("demiurge/rect.frag.glsl", Fragment, GLSLSource);

    format.appendPart("position", basics::Vec2f)
        ->nextGroup()
        ->setInstance()
        ->appendPart("rect_pos", basics::Vec4f)
        ->appendPart("rect_color", basics::Vec4f)
        ->appendPart("rect_depth", basics::Float)
        ->nextGroup()
        ->decideStruct()
        ->debugState();

    OMDemiurgeQuadChannel::init(uniformBuffer, target);
}
} // namespace openminecraft::renderer::common::demiurge::element
