#include "openminecraft/renderer/common/demiurge/element/om_demiurge_element_sector_channel.hpp"
#include "openminecraft/renderer/common/basics/om_vertex_format.hpp"

namespace openminecraft::renderer::common::demiurge::element
{
void OMDemiurgeSectorChannel::init(OMRendererBuffer *uniformBuffer, OMRendererRenderTarget *target)
{
    vtxShader = renderer->shaderManager.preprocess("demiurge/sector.vert.glsl", Vertex, GLSLSource);
    frgShader = renderer->shaderManager.preprocess("demiurge/sector.frag.glsl", Fragment, GLSLSource);

    format.appendPart("position", basics::Vec2f)
        ->nextGroup()
        ->setInstance()
        ->appendPart("sector_pos", basics::Vec4f)
        ->appendPart("sector_color", basics::Vec4f)
        ->appendPart("sector_rotation", basics::Vec4f)
        ->appendPart("sector_radius", basics::Float)
        ->appendPart("sector_beginangle", basics::Float)
        ->appendPart("sector_endangle", basics::Float)
        ->appendPart("sector_factor", basics::Float)
        ->appendPart("sector_depth", basics::Float)
        ->nextGroup()
        ->decideStruct()
        ->debugState();

    OMDemiurgeQuadChannel::init(uniformBuffer, target);
}
} // namespace openminecraft::renderer::common::demiurge::element
