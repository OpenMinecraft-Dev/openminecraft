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

    blurTemp = new wrap::OMRendererTempTarget(renderer, "blur.temp");
    blurTemp->construct(renderer->getExtent());
    blurTemp2 = new wrap::OMRendererTempTarget(renderer, "blur.temp2");
    blurTemp2->construct(renderer->getExtent());

    const std::string blurTagPrefix = "blur." + std::to_string(nextObjectTagId());
    blurArgs = OMRendererBufferRef(renderer, blurTagPrefix + ".args");
    renderer->registerBuffer(blurArgs.getTag(), Uniform, sizeof(OMRendererBlurArg));
    blurArgs2 = OMRendererBufferRef(renderer, blurTagPrefix + ".args2");
    renderer->registerBuffer(blurArgs2.getTag(), Uniform, sizeof(OMRendererBlurArg));

    biltPipeline = renderer->createPipeline()
                       ->input(ImageSampler)
                       ->inputName("inTexture")
                       ->output(blurTemp2->target())
                       ->shader(renderer->shaderManager.preprocess("core/bilt.frag.glsl", Fragment, GLSLSource, format))
                       ->shader(renderer->shaderManager.preprocess("core/bilt2.vert.glsl", Vertex, GLSLSource, format))
                       ->format(format)
                       ->blendFunc({SrcAlpha, OneMinusSrcAlpha, SrcAlpha, OneMinusSrcAlpha})
                       ->blend(true)
                       ->depth(false, false)
                       ->buildN();
    biltResources = biltPipeline->createResourceSet();
    blurp1Pipeline =
        renderer->createPipeline()
            ->input(UniformBuffer)
            ->inputName("BlurArgs")
            ->input(ImageSampler)
            ->inputName("inTexture")
            ->output(blurTemp->target())
            ->shader(renderer->shaderManager.preprocess("core/effects/blur.frag.glsl", Fragment, GLSLSource, format))
            ->shader(renderer->shaderManager.preprocess("core/effects/blur.vert.glsl", Vertex, GLSLSource, format))
            ->format(format)
            ->blendFunc({SrcAlpha, OneMinusSrcAlpha, SrcAlpha, OneMinusSrcAlpha})
            ->blend(true)
            ->depth(false, false)
            ->buildN();
    blurp1Resources = blurp1Pipeline->createResourceSet();
    blurp1Resources->bindInput(0, blurArgs);
    blurp2Pipeline =
        renderer->createPipeline()
            ->input(UniformBuffer)
            ->inputName("BlurArgs")
            ->input(ImageSampler)
            ->inputName("inTexture")
            ->output(blurTemp2->target())
            ->shader(renderer->shaderManager.preprocess("core/effects/blur.frag.glsl", Fragment, GLSLSource, format))
            ->shader(renderer->shaderManager.preprocess("core/effects/blur.vert.glsl", Vertex, GLSLSource, format))
            ->format(format)
            ->blendFunc({SrcAlpha, OneMinusSrcAlpha, SrcAlpha, OneMinusSrcAlpha})
            ->blend(true)
            ->depth(false, false)
            ->buildN();
    blurp2Resources = blurp2Pipeline->createResourceSet();
    blurp2Resources->bindInput(0, blurArgs2);

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
    composeResources = composePipeline->createResourceSet();
}
OMRendererBlurHandler::~OMRendererBlurHandler()
{
    delete biltResources;
    delete blurp1Resources;
    delete blurp2Resources;
    delete composeResources;
    delete blurp1Pipeline;
    delete blurp2Pipeline;
    delete composePipeline;
    delete biltPipeline;
    delete blurTemp;
    delete blurTemp2;
}

auto OMRendererBlurHandler::firstLayerTask(OMRendererTask *pre, std::string name) -> OMRendererTask *
{
    auto task = renderer->createTask(name)->target(blurTemp2->target())->pipeline(biltPipeline)->resourceSet(biltResources)->drawN(6);
    for (int i = 0; i < passes; ++i)
    {
        task->target(blurTemp->target())
            ->pipeline(blurp1Pipeline)
            ->resourceSet(blurp1Resources)
            ->drawN(6)
            ->target(blurTemp2->target())
            ->pipeline(blurp2Pipeline)
            ->resourceSet(blurp2Resources)
            ->drawN(6);
    }
    task->finishN();

    return task;
}
auto OMRendererBlurHandler::secondLayerTask(OMRendererTask *task) -> OMRendererTask *
{
    return task->pipeline(composePipeline)->resourceSet(composeResources)->drawN(6);
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
    blurp1Resources->bindInput(1, blurTemp2->colorTexture());
    blurp2Resources->bindInput(1, blurTemp->colorTexture());
    composeResources->bindInput(1, blurTemp2->colorTexture());
}

void OMRendererBlurHandler::bind(OMRendererTexture *uplayer, OMRendererTexture *bottomLayer)
{
    biltResources->bindInput(0, bottomLayer);
    composeResources->bindInput(0, uplayer);
}
} // namespace openminecraft::renderer::common::wrap
