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

    // 剪裁通道只负责 discard + 写深度，颜色输出必须被彻底忽略：
    // writeColor = false 在 Vulkan 里是 colorWriteMask = 0，在 OpenGL 里是 glColorMask(GL_FALSE, ...)，
    // 这样 color attachment 完全不会被写，也就不存在"未定义颜色"的 UB。
    initPipeline(uniformBuffer, target, false);
}
} // namespace openminecraft::renderer::common::demiurge::element
