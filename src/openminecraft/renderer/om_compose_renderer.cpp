#include "openminecraft-shell/renderer/om_compose_renderer.hpp"
#include "openminecraft/renderer/common/basics/om_vertex_format.hpp"
#include "openminecraft/renderer/common/om_renderer_handler.hpp"
#include "openminecraft/renderer/common/wrap/om_renderer_blur.hpp"
#include "openminecraft/renderer/common/wrap/om_renderer_temptarget.hpp"
#include <cstdlib>
#include <fstream>
#include <typeinfo>
#include <vector>

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
    blurHandler->update({16.0f, wrap::Gaussian});
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
    // 调试钩子：OM_DUMP_COMPOSE=/path/compose.ppm 导出合成后的最终画面
    static const char *dumpPath = std::getenv("OM_DUMP_COMPOSE");
    static bool dumped = false;
    static long long frame = 0;

    ++frame;

    if (!dumpPath || dumped || frame < 120 || !uplayerTarget || !uplayerTarget->colorTexture)
    {
        return;
    }

    dumped = true;

    auto *tex = uplayerTarget->colorTexture;
    auto data = tex->fetchData(0);
    if (!data)
    {
        return;
    }

    const uint32_t w = static_cast<uint32_t>(tex->width);
    const uint32_t h = static_cast<uint32_t>(tex->height);
    const auto *px = static_cast<const uint8_t *>(data.get());
    const bool flipY = std::string(typeid(*tex).name()).find("OpenGL") != std::string::npos;

    std::ofstream out(dumpPath, std::ios::binary);
    out << "P6\n" << w << " " << h << "\n255\n";

    std::vector<char> row(static_cast<size_t>(w) * 3);
    for (uint32_t y = 0; y < h; ++y)
    {
        const uint32_t sy = flipY ? (h - 1 - y) : y;
        for (uint32_t x = 0; x < w; ++x)
        {
            const uint8_t *p = px + (static_cast<size_t>(sy) * w + x) * 4;
            row[static_cast<size_t>(x) * 3 + 0] = static_cast<char>(p[0]);
            row[static_cast<size_t>(x) * 3 + 1] = static_cast<char>(p[1]);
            row[static_cast<size_t>(x) * 3 + 2] = static_cast<char>(p[2]);
        }
        out.write(row.data(), static_cast<std::streamsize>(row.size()));
    }
}
} // namespace openminecraft::shell::renderer
