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

    instanceBuffer = renderer->allocateBuffer(InstanceData, 8);

    pipeline = renderer->createPipeline()
                   ->input(UniformBuffer)
                   ->inputName("ScreenData")
                   ->output(target)
                   ->shader(frgShader)
                   ->shader(vtxShader)
                   ->format(format)
                   ->blendFunc({Zero, One, Zero, One})
                   ->blend(true)
                   ->depth(true, true)
                   ->depthOp(LessOrEqual)
                   ->buildN();
    pipeline->bindInput(0, uniformBuffer);
}
} // namespace openminecraft::renderer::common::demiurge::element
