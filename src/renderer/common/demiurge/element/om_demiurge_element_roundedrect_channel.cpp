#include "openminecraft/renderer/common/demiurge/element/om_demiurge_element_roundedrect_channel.hpp"
#include "openminecraft/renderer/common/basics/om_vertex_format.hpp"
#include "openminecraft/renderer/common/demiurge/element/om_demiurge_element_quad_channel.hpp"
#include "openminecraft/renderer/common/om_renderer_pipeline.hpp"
#include "openminecraft/vfs/om_vfs_base.hpp"
#include "openminecraft/io/om_io_utils.hpp"

namespace openminecraft::renderer::common::demiurge::element
{
void OMDemiurgeRoundedRectChannel::init(OMRendererBuffer *uniformBuffer, OMRendererRenderTarget *target)
{
#define shaderDef(name, filename, type)                                                                                \
    {                                                                                                                  \
        auto target = vfs::fsfetch(fmt::format("/bootassets/openminecraft-renderer/shaders/{}", filename));            \
        name = std::make_shared<OMShader>(GLSLSource, io::readOnce(target.get()), filename, "main", type);             \
    }
    shaderDef(vtxShader, "demiurge/roundedrect.vert.glsl", Vertex);
    shaderDef(frgShader, "demiurge/roundedrect.frag.glsl", Fragment);

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
