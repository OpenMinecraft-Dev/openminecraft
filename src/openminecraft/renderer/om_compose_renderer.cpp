#include "openminecraft-shell/renderer/om_compose_renderer.hpp"
#include "openminecraft/renderer/common/basics/om_vertex_format.hpp"
#include "openminecraft/renderer/common/om_renderer_handler.hpp"
#include "openminecraft/renderer/common/wrap/om_renderer_blur.hpp"
#include "openminecraft/renderer/common/wrap/om_renderer_temptarget.hpp"

using namespace openminecraft::renderer::common;

namespace openminecraft::shell::renderer
{
OMComposeRenderer::OMComposeRenderer(openminecraft::renderer::OMRenderer *renderer,
                                     openminecraft::renderer::common::wrap::OMRendererTempTarget *surface,
                                     openminecraft::renderer::common::wrap::OMRendererTempTarget *overlay,
                                     openminecraft::renderer::common::wrap::OMRendererTempTarget *scene,
                                     openminecraft::renderer::common::wrap::OMRendererTempTarget *surfaceBackground)
    : openminecraft::renderer::common::OMRendererHandler(renderer), overlay(overlay), scene(scene), surface(surface),
      surfaceBackground(surfaceBackground)
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

    backgroundPipeline =
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
    blurHandler->update({1.0f, wrap::Triangular});

    overlayBlurHandler = std::make_shared<wrap::OMRendererBlurHandler>(renderer, 1);
    renderer->registerHandler(overlayBlurHandler);
    overlayBlurHandler->update({1.0f, wrap::Triangular});
}

OMComposeRenderer::~OMComposeRenderer()
{
    delete mainPipeline;
    delete backgroundPipeline;
    delete uplayerTarget;
}

void OMComposeRenderer::submitTasks()
{
    uplayerTarget->construct(renderer->getExtent());

    mainPipeline->bindInput(0, scene->colorTexture);
    backgroundPipeline->bindInput(0, surfaceBackground->colorTexture);
    blurHandler->bind(surface->colorTexture, surfaceBackground->colorTexture);
    overlayBlurHandler->bind(overlay->colorTexture, scene->colorTexture);

    auto voxel = renderer->fetchTask("voxel");
    auto uiSurface = renderer->fetchTask("demiurgeui_compose_surface");
    auto uiBackground = renderer->fetchTask("demiurgeui_compose_surface_background");
    auto uiDebug = renderer->fetchTask("demiurgeui_compose_debughud");

    auto bgBlur = blurHandler->firstLayerTask(voxel, "demiurgeui_bgblur");
    bgBlur->dependOn(voxel);
    if (uiBackground)
    {
        bgBlur->dependOn(uiBackground);
    }
    if (uiSurface)
    {
        bgBlur->dependOn(uiSurface);
    }

    auto overlayBlur = overlayBlurHandler->firstLayerTask(voxel, "demiurgeui_overlayblur");
    overlayBlur->dependOn(voxel);
    if (uiDebug)
    {
        overlayBlur->dependOn(uiDebug);
    }

    auto main = renderer->createTask("main");
    main->dependOn(bgBlur);
    main->dependOn(overlayBlur);
    if (uiSurface)
    {
        main->dependOn(uiSurface);
    }
    if (uiBackground)
    {
        main->dependOn(uiBackground);
    }
    if (uiDebug)
    {
        main->dependOn(uiDebug);
    }
    main->target(renderer->getDefaultRenderTarget())->pipeline(mainPipeline)->drawN(6);

    overlayBlurHandler->secondLayerTask(main)->pipeline(backgroundPipeline)->drawN(6);
    blurHandler->secondLayerTask(main)->finishN();
}
void OMComposeRenderer::beforeFrame()
{
}
void OMComposeRenderer::afterFrame()
{
}
} // namespace openminecraft::shell::renderer
