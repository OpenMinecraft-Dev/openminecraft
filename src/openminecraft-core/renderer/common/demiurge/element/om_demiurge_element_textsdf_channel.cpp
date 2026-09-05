#include "openminecraft/renderer/common/demiurge/element/om_demiurge_element_textsdf_channel.hpp"
#include "openminecraft/renderer/common/basics/om_vertex_format.hpp"
#include "openminecraft/renderer/common/om_renderer_buffer.hpp"
#include "openminecraft/renderer/common/om_renderer_pipeline.hpp"
#include "openminecraft/renderer/common/om_renderer_shader.hpp"

namespace openminecraft::renderer::common::demiurge::element
{
auto OMDemiurgeTextSdfChannel::storeGlyph(geom::OMFontSetShapeResult r) -> int
{
    auto gid = static_cast<uint64_t>(r.fontId) << 32 | r.glyphId;
    if (glyphOffsets.count(gid))
    {
        return glyphOffsets[gid];
    }

    auto gdata = fontSet->compile(r);
    auto offset = glyphData.size();
    glyphData.insert(glyphData.end(), gdata.begin(), gdata.end());

    if (glyphBuffer->length < glyphData.size() * sizeof(float))
    {
        delete glyphBuffer;
        glyphBuffer = renderer->allocateBuffer(UniformTexel, 8 * glyphData.size() * sizeof(float));
        pipeline->bindInput(1, glyphBuffer);
        glyphBuffer->updateDataPart(glyphData.data(), 0, glyphData.size() * sizeof(float));
        recreation();
    }
    else
    {
        glyphBuffer->updateDataPart(&glyphData[offset], offset * sizeof(float), gdata.size() * sizeof(float));
    }

    glyphOffsets[gid] = offset;

    return offset;
}
void OMDemiurgeTextSdfChannel::init(OMRendererBuffer *uniform, OMRendererRenderTarget *target)
{
    format.setInstance()
        ->appendPart("inTextPos", basics::Vec4f)
        ->appendPart("inTextColor", basics::Vec4f)
        ->appendPart("inTextDepth", basics::Float)
        ->appendPart("inTextFactor", basics::Float)
        ->appendPart("inTextGlyphId", basics::Integer)
        ->nextGroup()
        ->decideStruct();

    vtxShader = renderer->shaderManager.preprocess("demiurge/textsdf.vert.glsl", Vertex, GLSLSource, format);
    frgShader = renderer->shaderManager.preprocess("demiurge/textsdf.frag.glsl", Fragment, GLSLSource, format);

    instanceBuffer = renderer->allocateBuffer(InstanceData, 8);

    glyphBuffer = renderer->allocateBuffer(UniformTexel, 1024 * sizeof(float));

    pipeline = renderer->createPipeline()
                   ->input(UniformBuffer)
                   ->inputName("ScreenData")
                   ->input(UniformTexelBuffer)
                   ->inputName("GlyphData")
                   ->output(target)
                   ->shader(frgShader)
                   ->shader(vtxShader)
                   ->format(format)
                   ->blendFunc({SrcAlpha, OneMinusSrcAlpha, One, OneMinusSrcAlpha})
                   ->blend(true)
                   ->depth(true, true)
                   ->depthOp(LessOrEqual)
                   ->buildN();
    pipeline->bindInput(0, uniform);
    pipeline->bindInput(1, glyphBuffer);
}
} // namespace openminecraft::renderer::common::demiurge::element
