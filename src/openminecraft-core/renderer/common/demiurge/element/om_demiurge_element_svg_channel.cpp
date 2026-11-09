#include "openminecraft/renderer/common/demiurge/element/om_demiurge_element_svg_channel.hpp"
#include "openminecraft/binary/om_bin_hash.hpp"
#include "openminecraft/geom/om_svg_structure.hpp"
#include "openminecraft/renderer/common/basics/om_vertex_format.hpp"
#include "openminecraft/renderer/common/om_renderer_buffer.hpp"
#include "openminecraft/renderer/common/om_renderer_pipeline.hpp"
#include "openminecraft/renderer/common/om_renderer_shader.hpp"

namespace openminecraft::renderer::common::demiurge::element
{
auto OMDemiurgeSvgChannel::storeGlyph(std::string s, glm::vec2 siz) -> int
{
    auto hsh = binary::hash::hash_compile_time(s.c_str());
    if (glyphOffsets.count(hsh))
    {
        return glyphOffsets[hsh];
    }

    auto gdata = geom::svg::compile(geom::svg::mergeTo(geom::svg::parseSvgPath(s)), siz);
    auto offset = glyphData.size();
    glyphData.insert(glyphData.end(), gdata.begin(), gdata.end());

    if (glyphBuffer->length < glyphData.size() * sizeof(float))
    {
        renderer->unregisterBuffer(glyphBuffer.getTag());
        glyphTag = baseGlyphTag + ".g" + std::to_string(++glyphGeneration);
        glyphBuffer = wrap::OMBufferRef(renderer, glyphTag);
        renderer->registerBuffer(glyphTag, UniformTexel, 8 * glyphData.size() * sizeof(float));
        pipeline->bindInput(1, glyphBuffer);
        glyphBuffer->updateDataPart(glyphData.data(), 0, glyphData.size() * sizeof(float));
        recreation();
    }
    else
    {
        glyphBuffer->updateDataPart(&glyphData[offset], offset * sizeof(float), gdata.size() * sizeof(float));
    }

    glyphOffsets[hsh] = offset;

    return offset;
}
void OMDemiurgeSvgChannel::init(const std::string &uniformTag, const std::string &targetTag)
{
    const auto uniform = wrap::OMBufferRef(renderer, uniformTag);
    const auto target = wrap::OMRenderTargetRef(renderer, targetTag);

    format.setInstance()
        ->appendPart("inSvgPos", basics::Vec4f)
        ->appendPart("inSvgColor", basics::Vec4f)
        ->appendPart("inSvgDepth", basics::Float)
        ->appendPart("inSvgFactor", basics::Float)
        ->appendPart("inSvgGlyphId", basics::Integer)
        ->appendPart("inSvgScale", basics::Vec2f)
        ->nextGroup()
        ->decideStruct();

    vtxShader = renderer->shaderManager.preprocess("demiurge/svg.vert.glsl", Vertex, GLSLSource, format);
    frgShader = renderer->shaderManager.preprocess("demiurge/svg.frag.glsl", Fragment, GLSLSource, format);

    resetInstanceBuffer(8);

    if (!glyphBuffer)
    {
        glyphTag = baseGlyphTag;
        glyphBuffer = wrap::OMBufferRef(renderer, glyphTag);
        renderer->registerBuffer(glyphTag, UniformTexel, 1024 * sizeof(float));
    }

    pipeline = renderer->createPipeline()
                   ->input(UniformBuffer)
                   ->inputName("ScreenData")
                   ->input(UniformTexelBuffer)
                   ->inputName("SvgData")
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
