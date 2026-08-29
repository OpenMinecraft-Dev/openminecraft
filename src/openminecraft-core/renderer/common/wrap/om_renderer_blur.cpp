#include "openminecraft/renderer/common/wrap/om_renderer_blur.hpp"
#include "openminecraft/renderer/common/om_renderer_buffer.hpp"
#include "openminecraft/renderer/common/om_renderer_pipeline.hpp"
#include "openminecraft/renderer/common/om_renderer_shader.hpp"
#include "openminecraft/renderer/common/om_renderer_task.hpp"
#include "openminecraft/renderer/om_renderer_layer.hpp"

namespace openminecraft::renderer::common::wrap
{
static int boxblurIndex = 0;
OMRendererBlurHandler::OMRendererBlurHandler(OMRenderer *renderer, int passes) : OMRendererHandler(renderer)
{
    this->renderer = renderer;
    this->passes = passes;

    format.nextGroup()->decideStruct();

    blurTemp = new wrap::OMRendererTempTarget(renderer);
    blurTemp->construct(renderer->getExtent());
    blurTemp2 = new wrap::OMRendererTempTarget(renderer);
    blurTemp2->construct(renderer->getExtent());

    blurArgs = renderer->allocateBuffer(Uniform, sizeof(OMRendererBlurArg));
    blurArgs2 = renderer->allocateBuffer(Uniform, sizeof(OMRendererBlurArg));

    biltPipeline = renderer->createPipeline()
                       ->input(ImageSampler)
                       ->inputName("inTexture")
                       ->output(blurTemp2->target)
                       ->shader(renderer->shaderManager.preprocess("core/bilt.frag.glsl", Fragment, GLSLSource, format))
                       ->shader(renderer->shaderManager.preprocess("core/bilt2.vert.glsl", Vertex, GLSLSource, format))
                       ->format(format)
                       ->blendFunc({SrcAlpha, OneMinusSrcAlpha, SrcAlpha, OneMinusSrcAlpha})
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
            ->shader(renderer->shaderManager.preprocess("core/effects/blur.frag.glsl", Fragment, GLSLSource, format))
            ->shader(renderer->shaderManager.preprocess("core/effects/blur.vert.glsl", Vertex, GLSLSource, format))
            ->format(format)
            ->blendFunc({SrcAlpha, OneMinusSrcAlpha, SrcAlpha, OneMinusSrcAlpha})
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
            ->shader(renderer->shaderManager.preprocess("core/effects/blur.frag.glsl", Fragment, GLSLSource, format))
            ->shader(renderer->shaderManager.preprocess("core/effects/blur.vert.glsl", Vertex, GLSLSource, format))
            ->format(format)
            ->blendFunc({SrcAlpha, OneMinusSrcAlpha, SrcAlpha, OneMinusSrcAlpha})
            ->blend(true)
            ->depth(false, false)
            ->buildN();
    blurp2Pipeline->bindInput(0, blurArgs2);

    composePipeline =
        renderer->createPipeline()
            ->input(ImageSampler)
            ->inputName("inTextureFg")
            ->input(ImageSampler)
            ->inputName("inTexture")
            ->output(renderer->getDefaultRenderTarget())
            ->shader(renderer->shaderManager.preprocess("core/effects/compose.frag.glsl", Fragment, GLSLSource, format))
            ->shader(renderer->shaderManager.preprocess("core/effects/compose.vert.glsl", Vertex, GLSLSource, format))
            ->format(format)
            ->blendFunc({SrcAlpha, OneMinusSrcAlpha, SrcAlpha, OneMinusSrcAlpha})
            ->blend(true)
            ->depth(false, false)
            ->buildN();
}
OMRendererBlurHandler::~OMRendererBlurHandler()
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

auto OMRendererBlurHandler::firstLayerTask(OMRendererTask *pre) -> OMRendererTask *
{
    auto task = renderer->createTask(fmt::format("boxblur_{}", ++boxblurIndex))
                    ->target(blurTemp2->target)
                    ->pipeline(biltPipeline)
                    ->drawN(6);
    for (int i = 0; i < passes; ++i)
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
auto OMRendererBlurHandler::secondLayerTask(OMRendererTask *task) -> OMRendererTask *
{
    return task->pipeline(composePipeline)->drawN(6);
}
void OMRendererBlurHandler::update(OMRendererBlurArg a)
{
    auto temp = OMRendererBlurArg{a.radius, a.blurType, 0.0f};
    auto temp2 = OMRendererBlurArg{a.radius, a.blurType, 1.0f};
    blurArgs->updateData(&temp);
    blurArgs2->updateData(&temp2);
}

void OMRendererBlurHandler::beforeFrame()
{
}
void OMRendererBlurHandler::afterFrame()
{
}
void OMRendererBlurHandler::submitTasks()
{
    blurTemp->construct(renderer->getExtent());
    blurTemp2->construct(renderer->getExtent());
    blurp1Pipeline->bindInput(1, blurTemp2->colorTexture);
    blurp2Pipeline->bindInput(1, blurTemp->colorTexture);
    composePipeline->bindInput(1, blurTemp2->colorTexture);
}

void OMRendererBlurHandler::bind(OMRendererTexture *uplayer, OMRendererTexture *bottomLayer)
{
    biltPipeline->bindInput(0, bottomLayer);
    composePipeline->bindInput(0, uplayer);
}
} // namespace openminecraft::renderer::common::wrap
