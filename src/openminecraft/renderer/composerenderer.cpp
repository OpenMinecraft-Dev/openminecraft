#include "openminecraft-shell/renderer/composerenderer.hpp"
#include "openminecraft/renderer/common/basics/om_vertex_format.hpp"
#include "openminecraft/renderer/common/om_renderer_handler.hpp"
#include "openminecraft/renderer/common/wrap/om_renderer_blur.hpp"
#include "openminecraft/renderer/common/wrap/om_renderer_temptarget.hpp"

using namespace openminecraft::renderer::common;

namespace openminecraftshell::renderer
{
OMComposeRenderer::OMComposeRenderer(openminecraft::renderer::OMRenderer *renderer,
                                     openminecraft::renderer::common::wrap::OMRendererTempTarget *surface,
                                     openminecraft::renderer::common::wrap::OMRendererTempTarget *overlay,
                                     openminecraft::renderer::common::wrap::OMRendererTempTarget *scene)
    : openminecraft::renderer::common::OMRendererHandler(renderer), overlay(overlay), scene(scene), surface(surface)
{
    this->renderer = renderer;

    basics::OMVertexFormat format;
    format.nextGroup()->decideStruct();

    uplayerTarget = new wrap::OMRendererTempTarget(renderer);
    uplayerTarget->construct(renderer->getExtent());

    mainPipeline = renderer->createPipeline()
                       ->input(ImageSampler)
                       ->inputName("inTexture")
                       ->output(renderer->getDefaultRenderTarget())
                       ->shader(renderer->shaderManager.preprocess("core/bilt.frag.glsl", Fragment, GLSLSource, format))
                       ->shader(renderer->shaderManager.preprocess("core/bilt.vert.glsl", Vertex, GLSLSource, format))
                       ->format(format)
                       ->blendFunc({SrcAlpha, OneMinusSrcAlpha, SrcAlpha, OneMinusSrcAlpha})
                       ->blend(true)
                       ->depth(false, false)
                       ->buildN();

    overlayPipeline =
        renderer->createPipeline()
            ->input(ImageSampler)
            ->inputName("inTexture")
            ->output(renderer->getDefaultRenderTarget())
            ->shader(renderer->shaderManager.preprocess("core/bilt.frag.glsl", Fragment, GLSLSource, format))
            ->shader(renderer->shaderManager.preprocess("core/bilt.vert.glsl", Vertex, GLSLSource, format))
            ->format(format)
            ->blendFunc({SrcAlpha, OneMinusSrcAlpha, SrcAlpha, OneMinusSrcAlpha})
            ->blend(true)
            ->depth(false, false)
            ->buildN();

    blurHandler = std::make_shared<wrap::OMRendererBlurHandler>(renderer, 1);
    renderer->registerHandler(blurHandler);
    blurHandler->update({32.0f, wrap::Gaussian});
}

OMComposeRenderer::~OMComposeRenderer()
{
    delete mainPipeline;
    delete overlayPipeline;
    delete uplayerTarget;
}

void OMComposeRenderer::submitTasks()
{
    uplayerTarget->construct(renderer->getExtent());

    mainPipeline->bindInput(0, scene->colorTexture);
    overlayPipeline->bindInput(0, overlay->colorTexture);
    blurHandler->bind(surface->colorTexture, scene->colorTexture);

    auto voxel = renderer->fetchTask("voxel");
    blurHandler
        ->secondLayerTask(renderer->createTask("main")
                              ->dependOn(blurHandler->firstLayerTask(voxel)->dependOn(voxel))
                              ->dependOn(renderer->fetchTask("demiurgeui_compose_debughud"))
                              ->dependOn(renderer->fetchTask("demiurgeui_compose_surface"))
                              ->target(renderer->getDefaultRenderTarget())
                              ->pipeline(mainPipeline)
                              ->drawN(6)
                              ->pipeline(overlayPipeline)
                              ->drawN(6))
        ->finishN();
}
void OMComposeRenderer::beforeFrame()
{
}
void OMComposeRenderer::afterFrame()
{
}
} // namespace openminecraftshell::renderer
