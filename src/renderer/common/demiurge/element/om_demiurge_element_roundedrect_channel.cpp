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

    format.appendPart("position", basics::Vec2f);
    format.nextGroup();
    format.setInstance();
    format.appendPart("rect_pos", basics::Vec4f);
    format.appendPart("rect_color", basics::Vec4f);
    format.appendPart("rect_radius", basics::Vec4f);
    format.appendPart("rect_factor", basics::Float);
    format.appendPart("rect_depth", basics::Float);
    format.nextGroup();
    format.decideStruct();
    format.debugState();

    OMDemiurgeQuadChannel::init(uniformBuffer, target);
}
} // namespace openminecraft::renderer::common::demiurge::element
