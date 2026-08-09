#include "openminecraft/renderer/common/demiurge/element/om_demiurge_element_sector_channel.hpp"
#include "openminecraft/renderer/common/basics/om_vertex_format.hpp"

namespace openminecraft::renderer::common::demiurge::element
{
void OMDemiurgeSectorChannel::init(OMRendererBuffer *uniformBuffer, OMRendererRenderTarget *target)
{
    format.appendPart("inPosition", basics::Vec2f)
        ->nextGroup()
        ->setInstance()
        ->appendPart("inSectorPos", basics::Vec4f)
        ->appendPart("inSectorColor", basics::Vec4f)
        ->appendPart("inSectorRotation", basics::Vec4f)
        ->appendPart("inSectorBeginAngle", basics::Float)
        ->appendPart("inSectorEndAngle", basics::Float)
        ->appendPart("inSectorFactor", basics::Float)
        ->appendPart("inSectorDepth", basics::Float)
        ->nextGroup()
        ->decideStruct();

    vtxShader = renderer->shaderManager.preprocess("demiurge/sector.vert.glsl", Vertex, GLSLSource, format);
    frgShader = renderer->shaderManager.preprocess("demiurge/sector.frag.glsl", Fragment, GLSLSource, format);

    OMDemiurgeQuadChannel::init(uniformBuffer, target);
}
} // namespace openminecraft::renderer::common::demiurge::element
