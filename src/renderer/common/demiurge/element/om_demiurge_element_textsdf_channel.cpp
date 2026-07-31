#include "openminecraft/renderer/common/demiurge/element/om_demiurge_element_textsdf_channel.hpp"
#include "openminecraft/renderer/common/basics/om_vertex_format.hpp"
#include "openminecraft/renderer/common/om_renderer_buffer.hpp"
#include "openminecraft/renderer/common/om_renderer_pipeline.hpp"
#include "openminecraft/vfs/om_vfs_base.hpp"
#include "openminecraft/io/om_io_utils.hpp"

namespace openminecraft::renderer::common::demiurge::element
{
void OMDemiurgeTextSdfChannel::bakeGlyph(fontproc::OMFontSetShapeResult r)
{
}
void OMDemiurgeTextSdfChannel::init(OMRendererBuffer *uniform, OMRendererRenderTarget *target)
{
#define shaderDef(name, filename, type)                                                                                \
    {                                                                                                                  \
        auto target = vfs::fsfetch(fmt::format("/bootassets/openminecraft-renderer/shaders/{}", filename));            \
        name = std::make_shared<OMShader>(GLSLSource, io::readOnce(target.get()), filename, "main", type);             \
    }
    shaderDef(vtxShader, "demiurge/textsdf.vert.glsl", Vertex);
    shaderDef(frgShader, "demiurge/textsdf.frag.glsl", Fragment);

    format.appendPart("position", basics::Vec2f)
        ->nextGroup()
        ->setInstance()
        ->appendPart("textsdf_pos", basics::Vec4f)
        ->appendPart("textsdf_color", basics::Vec4f)
        ->appendPart("textsdf_depth", basics::Float)
        ->appendPart("textsdf_height", basics::Float)
        ->appendPart("textsdf_factor", basics::Float)
        ->appendPart("textsdf_glyphIndex", basics::Integer)
        ->nextGroup()
        ->decideStruct()
        ->debugState();

    quadBuffer = renderer->allocateBuffer(VertexData, 4 * sizeof(glm::vec2));
    quadBuffer->updateData(std::array<glm::vec2, 4>{{{0.0f, 0.0f}, {0.0f, 1.0f}, {1.0f, 1.0f}, {1.0f, 0.0f}}}.data());
    quadIndex = renderer->allocateBuffer(VertexIndex, 6 * sizeof(uint32_t));
    quadIndex->updateData(std::array<uint32_t, 6>{{0, 1, 2, 2, 3, 0}}.data());
    instanceBuffer = renderer->allocateBuffer(InstanceData, 8);

    glyphBuffer = renderer->allocateBuffer(ShaderStorage, 1024 * sizeof(float));

    pipeline = renderer->createPipeline()
                   ->input(UniformBuffer)
                   ->input(ShaderStorageBuffer)
                   ->output(target)
                   ->shader(frgShader)
                   ->shader(vtxShader)
                   ->format(format)
                   ->blendFunc({Alpha, OneMinusAlpha, One, OneMinusAlpha})
                   ->blend(true)
                   ->depth(false, false)
                   ->buildN();
    pipeline->bindInput(0, uniform);
    pipeline->bindInput(1, glyphBuffer);
}
} // namespace openminecraft::renderer::common::demiurge::element
