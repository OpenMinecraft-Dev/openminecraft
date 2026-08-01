#include "openminecraft/renderer/common/demiurge/element/om_demiurge_element_roundedrect_channel.hpp"
#include "openminecraft/renderer/common/basics/om_vertex_format.hpp"
#include "openminecraft/renderer/common/demiurge/element/om_demiurge_element_quad_channel.hpp"
#include "openminecraft/renderer/common/om_renderer_pipeline.hpp"
#include "openminecraft/renderer/common/om_renderer_shader.hpp"

namespace openminecraft::renderer::common::demiurge::element
{
void OMDemiurgeRoundedRectChannel::init(OMRendererBuffer *uniformBuffer, OMRendererRenderTarget *target)
{
    vtxShader = renderer->shaderManager.preprocess("demiurge/roundedrect.vert.glsl", Vertex, GLSLSource);
    frgShader = renderer->shaderManager.preprocess("demiurge/roundedrect.frag.glsl", Fragment, GLSLSource);

    format.appendPart("position", basics::Vec2f)
        ->nextGroup()
        ->setInstance()
        ->appendPart("rrect_pos", basics::Vec4f)
        ->appendPart("rrect_color", basics::Vec4f)
        ->appendPart("rrect_radius", basics::Vec4f)
        ->appendPart("rrect_factor", basics::Float)
        ->appendPart("rrect_depth", basics::Float)
        ->nextGroup()
        ->decideStruct()
        ->debugState();

    OMDemiurgeQuadChannel::init(uniformBuffer, target);
}
} // namespace openminecraft::renderer::common::demiurge::element
