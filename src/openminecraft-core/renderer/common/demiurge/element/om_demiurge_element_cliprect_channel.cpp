#include "openminecraft/renderer/common/demiurge/element/om_demiurge_element_cliprect_channel.hpp"
#include "openminecraft/renderer/common/om_renderer_pipeline.hpp"
#include "openminecraft/renderer/common/om_renderer_shader.hpp"

namespace openminecraft::renderer::common::demiurge::element
{
void OMDemiurgeClipRectChannel::init(OMRendererBuffer *uniformBuffer, OMRendererRenderTarget *target)
{
    format.setInstance()
        ->appendPart("inRectPos", basics::Vec4f)
        ->appendPart("inRectDepth", basics::Float)
        ->nextGroup()
        ->decideStruct();

    vtxShader = renderer->shaderManager.preprocess("demiurge/cliprect.vert.glsl", Vertex, GLSLSource, format);
    frgShader = renderer->shaderManager.preprocess("demiurge/cliprect.frag.glsl", Fragment, GLSLSource, format);

    OMDemiurgeQuadChannel::init(uniformBuffer, target);
}
} // namespace openminecraft::renderer::common::demiurge::element
