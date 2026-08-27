#include "openminecraft/renderer/common/wrap/om_renderer_boxblur.hpp"
#include "openminecraft/renderer/common/om_renderer_buffer.hpp"
#include "openminecraft/renderer/common/om_renderer_pipeline.hpp"
#include "openminecraft/renderer/common/om_renderer_shader.hpp"
#include "openminecraft/renderer/common/om_renderer_task.hpp"
#include "openminecraft/renderer/om_renderer_layer.hpp"

namespace openminecraft::renderer::common::wrap
{
static int boxblurIndex = 0;
OMRendererBoxBlurHandler::OMRendererBoxBlurHandler(OMRenderer *renderer) : OMRendererHandler(renderer)
{
    this->renderer = renderer;

    format.nextGroup()->decideStruct();

    blurTemp = new wrap::OMRendererTempTarget(renderer);
    blurTemp->construct(renderer->getExtent());
    blurTemp2 = new wrap::OMRendererTempTarget(renderer);
    blurTemp2->construct(renderer->getExtent());

    blurArgs = renderer->allocateBuffer(Uniform, sizeof(OMRendererBoxBlurArg));
    blurArgs2 = renderer->allocateBuffer(Uniform, sizeof(OMRendererBoxBlurArg));

    biltPipeline = renderer->createPipeline()
                       ->input(ImageSampler)
                       ->inputName("inTexture")
                       ->output(blurTemp2->target)
                       ->shader(renderer->shaderManager.preprocess("core/bilt.frag.glsl", Fragment, GLSLSource, format))
                       ->shader(renderer->shaderManager.preprocess("core/bilt2.vert.glsl", Vertex, GLSLSource, format))
                       ->format(format)
                       ->blendFunc({Alpha, OneMinusAlpha, Alpha, OneMinusAlpha})
                       ->blend(true)
                       ->depth(false, false)
                       ->buildN();
    blurp1Pipeline =
        renderer->createPipeline()
            ->input(UniformBuffer)
            ->inputName("BlurArgs")
            ->input(ImageSampler)
            ->inputName("inTexture")
            ->output(blurTemp->target)
            ->shader(renderer->shaderManager.preprocess("core/effects/boxblur.frag.glsl", Fragment, GLSLSource, format))
            ->shader(renderer->shaderManager.preprocess("core/effects/boxblur.vert.glsl", Vertex, GLSLSource, format))
            ->format(format)
            ->blendFunc({Alpha, OneMinusAlpha, Alpha, OneMinusAlpha})
            ->blend(true)
            ->depth(false, false)
            ->buildN();
    blurp1Pipeline->bindInput(0, blurArgs);
    blurp2Pipeline =
        renderer->createPipeline()
            ->input(UniformBuffer)
            ->inputName("BlurArgs")
            ->input(ImageSampler)
            ->inputName("inTexture")
            ->output(blurTemp2->target)
            ->shader(renderer->shaderManager.preprocess("core/effects/boxblur.frag.glsl", Fragment, GLSLSource, format))
            ->shader(renderer->shaderManager.preprocess("core/effects/boxblur.vert.glsl", Vertex, GLSLSource, format))
            ->format(format)
            ->blendFunc({Alpha, OneMinusAlpha, Alpha, OneMinusAlpha})
            ->blend(true)
            ->depth(false, false)
            ->buildN();
    blurp2Pipeline->bindInput(0, blurArgs2);

    composePipeline = renderer->createPipeline()
                          ->input(ImageSampler)
                          ->inputName("inTextureFg")
                          ->input(ImageSampler)
                          ->inputName("inTexture")
                          ->output(renderer->getDefaultRenderTarget())
                          ->shader(renderer->shaderManager.preprocess("core/effects/boxblurcompose.frag.glsl", Fragment,
                                                                      GLSLSource, format))
                          ->shader(renderer->shaderManager.preprocess("core/effects/boxblurcompose.vert.glsl", Vertex,
                                                                      GLSLSource, format))
                          ->format(format)
                          ->blendFunc({Alpha, OneMinusAlpha, Alpha, OneMinusAlpha})
                          ->blend(true)
                          ->depth(false, false)
                          ->buildN();
}
OMRendererBoxBlurHandler::~OMRendererBoxBlurHandler()
{
    delete blurp1Pipeline;
    delete blurp2Pipeline;
    delete composePipeline;
    delete biltPipeline;
    delete blurArgs;
    delete blurArgs2;
    delete blurTemp;
    delete blurTemp2;
}

auto OMRendererBoxBlurHandler::firstLayerTask(OMRendererTask *pre) -> OMRendererTask *
{
    auto task = renderer->createTask(fmt::format("boxblur_{}", ++boxblurIndex))
                    ->target(blurTemp2->target)
                    ->pipeline(biltPipeline)
                    ->drawN(6);
    for (int i = 0; i < 1; ++i)
    {
        task->target(blurTemp->target)
            ->pipeline(blurp1Pipeline)
            ->drawN(6)
            ->target(blurTemp2->target)
            ->pipeline(blurp2Pipeline)
            ->drawN(6);
    }
    task->finishN();

    return task;
}
auto OMRendererBoxBlurHandler::secondLayerTask(OMRendererTask *task) -> OMRendererTask *
{
    return task->pipeline(composePipeline)->drawN(6);
}
void OMRendererBoxBlurHandler::update(OMRendererBoxBlurArg a)
{
    auto temp = OMRendererBoxBlurArg{a.radius, 0.0f};
    auto temp2 = OMRendererBoxBlurArg{a.radius, 1.0f};
    blurArgs->updateData(&temp);
    blurArgs2->updateData(&temp2);
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
    blurTemp2->construct(renderer->getExtent());
    blurp1Pipeline->bindInput(1, blurTemp2->colorTexture);
    blurp2Pipeline->bindInput(1, blurTemp->colorTexture);
    composePipeline->bindInput(1, blurTemp2->colorTexture);
}

void OMRendererBoxBlurHandler::bind(OMRendererTexture *uplayer, OMRendererTexture *bottomLayer)
{
    biltPipeline->bindInput(0, bottomLayer);
    composePipeline->bindInput(0, uplayer);
}
} // namespace openminecraft::renderer::common::wrap
