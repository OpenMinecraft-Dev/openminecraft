#include "openminecraft-shell/renderer/composerenderer.hpp"
#include "openminecraft/renderer/common/basics/om_vertex_format.hpp"
#include "openminecraft/renderer/common/om_renderer_handler.hpp"
#include "openminecraft/renderer/common/wrap/om_renderer_blur.hpp"

using namespace openminecraft::renderer::common;

namespace openminecraftshell::renderer
{
OMComposeRenderer::OMComposeRenderer(openminecraft::renderer::OMRenderer *renderer,
                                     std::function<openminecraft::renderer::common::OMRendererTexture *()> overlay,
                                     std::function<openminecraft::renderer::common::OMRendererTexture *()> scene)
    : openminecraft::renderer::common::OMRendererHandler(renderer)
{
    this->renderer = renderer;
    this->overlay = overlay;
    this->scene = scene;

    basics::OMVertexFormat format;
    format.nextGroup()->decideStruct();

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

    blurHandler = std::make_shared<wrap::OMRendererBlurHandler>(renderer, 1);
    renderer->registerHandler(blurHandler);
    blurHandler->update({64.0f, wrap::Gaussian});
}

OMComposeRenderer::~OMComposeRenderer()
{
    delete mainPipeline;
}

void OMComposeRenderer::submitTasks()
{
    mainPipeline->bindInput(0, scene());
    blurHandler->bind(overlay(), scene());

    auto voxel = renderer->fetchTask("voxel");
    blurHandler
        ->secondLayerTask(renderer->createTask("main")
                              ->dependOn(voxel)
                              ->dependOn(blurHandler->firstLayerTask(voxel))
                              ->dependOn(renderer->fetchTask("demiurgeui_compose"))
                              ->target(renderer->getDefaultRenderTarget())
                              ->pipeline(mainPipeline)
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
