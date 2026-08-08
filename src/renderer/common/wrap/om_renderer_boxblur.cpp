#include "openminecraft/renderer/common/wrap/om_renderer_boxblur.hpp"
#include "openminecraft/renderer/common/om_renderer_buffer.hpp"
#include "openminecraft/renderer/common/om_renderer_pipeline.hpp"
#include "openminecraft/renderer/common/om_renderer_task.hpp"
#include "openminecraft/renderer/om_renderer_layer.hpp"
#include <array>

namespace openminecraft::renderer::common::wrap
{
static int boxblurIndex = 0;
OMRendererBoxBlurHandler::OMRendererBoxBlurHandler(OMRenderer *renderer) : OMRendererHandler(renderer)
{
    this->renderer = renderer;

    outputBlurp2Frg = renderer->shaderManager.preprocess("core/effects/boxblurp2.frag.glsl", Fragment, GLSLSource);
    outputBlurp2Vtx = renderer->shaderManager.preprocess("core/effects/boxblurp2.vert.glsl", Vertex, GLSLSource);
    outputBlurp1Frg = renderer->shaderManager.preprocess("core/effects/boxblurp1.frag.glsl", Fragment, GLSLSource);
    outputBlurp1Vtx = renderer->shaderManager.preprocess("core/effects/boxblurp1.vert.glsl", Vertex, GLSLSource);

    format.appendPart("position", basics::Vec3f)
        ->appendPart("textureUV", basics::Vec2f)
        ->nextGroup()
        ->decideStruct()
        ->debugState();

    blurTemp = new wrap::OMRendererTempTarget(renderer);
    blurTemp->construct(renderer->getExtent());

    blurArgs = renderer->allocateBuffer(Uniform, sizeof(OMRendererBoxBlurArg));

    blurp1Pipeline = renderer->createPipeline()
                         ->input(UniformBuffer)
                         ->inputName("BlurArgs")
                         ->input(ImageSampler)
                         ->inputName("inTexture")
                         ->output(blurTemp->target)
                         ->shader(outputBlurp1Frg)
                         ->shader(outputBlurp1Vtx)
                         ->format(format)
                         ->blendFunc({Alpha, OneMinusAlpha, Alpha, OneMinusAlpha})
                         ->blend(true)
                         ->depth(false, false)
                         ->buildN();
    blurp1Pipeline->bindInput(0, blurArgs);
    blurp2Pipeline = renderer->createPipeline()
                         ->input(UniformBuffer)
                         ->inputName("BlurArgs")
                         ->input(ImageSampler)
                         ->inputName("inTextureFg")
                         ->input(ImageSampler)
                         ->inputName("inTexture")
                         ->output(renderer->getDefaultRenderTarget())
                         ->shader(outputBlurp2Frg)
                         ->shader(outputBlurp2Vtx)
                         ->format(format)
                         ->blendFunc({Alpha, OneMinusAlpha, Alpha, OneMinusAlpha})
                         ->blend(true)
                         ->depth(false, false)
                         ->buildN();
    blurp2Pipeline->bindInput(0, blurArgs);

    std::array<OMRendererBoxBlurVertex, 4> vtxs = {{
        {{-1.0f, -1.0f, 0.0f}, {0.0f, 0.0f}},
        {{-1.0f, 1.0f, 0.0f}, {0.0f, 1.0f}},
        {{1.0f, 1.0f, 0.0f}, {1.0f, 1.0f}},
        {{1.0f, -1.0f, 0.0f}, {1.0f, 0.0f}},
    }};
    std::array<uint32_t, 6> vtxi = {0, 1, 2, 2, 3, 0};

    quadVertex = renderer->allocateBuffer(VertexData, 4 * sizeof(OMRendererBoxBlurVertex));
    quadVertex->updateData(vtxs.data());
    quadIndex = renderer->allocateBuffer(VertexIndex, 6 * sizeof(uint32_t));
    quadIndex->updateData(vtxi.data());
}
OMRendererBoxBlurHandler::~OMRendererBoxBlurHandler()
{
    delete blurp1Pipeline;
    delete blurp2Pipeline;
    delete blurArgs;
    delete blurTemp;

    delete quadVertex;
    delete quadIndex;
}
auto OMRendererBoxBlurHandler::firstLayerTask(OMRendererTask *pre) -> OMRendererTask *
{
    return renderer->createTask(fmt::format("boxblur_{}", ++boxblurIndex))
        ->dependOn(pre)
        ->target(blurTemp->target)
        ->pipeline(blurp1Pipeline)
        ->vertexBuffer({quadVertex})
        ->indexBuffer(quadIndex)
        ->drawN(6)
        ->finishN();
    ;
}
auto OMRendererBoxBlurHandler::secondLayerTask(OMRendererTask *task) -> OMRendererTask *
{
    return task->pipeline(blurp2Pipeline)->vertexBuffer({quadVertex})->indexBuffer(quadIndex)->drawN(6);
}
void OMRendererBoxBlurHandler::update(OMRendererBoxBlurArg a)
{
    blurArgs->updateData(&a);
}

void OMRendererBoxBlurHandler::beforeFrame()
{
}
void OMRendererBoxBlurHandler::afterFrame()
{
}
void OMRendererBoxBlurHandler::submitTasks()
{
    blurTemp->construct(renderer->getExtent());
    blurp2Pipeline->bindInput(2, blurTemp->colorTexture);
}

void OMRendererBoxBlurHandler::bind(OMRendererTexture *uplayer, OMRendererTexture *bottomLayer)
{
    blurp2Pipeline->bindInput(1, uplayer);
    blurp1Pipeline->bindInput(1, bottomLayer);
}
} // namespace openminecraft::renderer::common::wrap
