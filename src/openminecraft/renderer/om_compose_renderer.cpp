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
                                     openminecraft::renderer::common::wrap::OMRendererTempTarget *scene)
    : openminecraft::renderer::common::OMRendererHandler(renderer), overlay(overlay), scene(scene), surface(surface)
{
    this->renderer = renderer;

    basics::OMVertexFormat format;
    format.nextGroup()->decideStruct();

    uplayerTarget = new wrap::OMRendererTempTarget(renderer, "compose.uplayer");
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

    // Debug HUD is composited with a plain blit (no blur).
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
    blurHandler->update({8.0f, wrap::Triangular});
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

    mainPipeline->bindInput(0, scene->colorTexture());
    overlayPipeline->bindInput(0, overlay->colorTexture());
    blurHandler->bind(surface->colorTexture(), scene->colorTexture());

    auto voxel = renderer->fetchTask("voxel");
    auto uiSurface = renderer->fetchTask("demiurgeui_compose_surface");
    auto uiDebug = renderer->fetchTask("demiurgeui_compose_debughud");

    auto bgBlur = blurHandler->firstLayerTask(voxel, "demiurgeui_bgblur");
    bgBlur->dependOn(voxel);
    if (uiSurface)
    {
        bgBlur->dependOn(uiSurface);
    }

    auto main = renderer->createTask("main");
    main->dependOn(bgBlur);
    if (uiSurface)
    {
        main->dependOn(uiSurface);
    }
    if (uiDebug)
    {
        main->dependOn(uiDebug);
    }
    main->target(renderer->getDefaultRenderTarget())->pipeline(mainPipeline)->drawN(6);

    main->pipeline(overlayPipeline)->drawN(6);
    blurHandler->secondLayerTask(main);

    main->finishN();
}
void OMComposeRenderer::beforeFrame()
{
}
void OMComposeRenderer::afterFrame()
{
}
} // namespace openminecraft::shell::renderer
