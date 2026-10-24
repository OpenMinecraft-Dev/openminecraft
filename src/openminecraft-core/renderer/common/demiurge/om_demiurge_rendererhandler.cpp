#include "openminecraft/renderer/common/demiurge/om_demiurge_rendererhandler.hpp"
#include <vector>
#include <cstdlib>
#include <typeinfo>
#include <fstream>
#include "openminecraft/geom/om_fontset.hpp"
#include "openminecraft/geom/om_svg_structure.hpp"
#include "openminecraft/renderer/common/basics/om_vertex_format.hpp"
#include "openminecraft/renderer/common/demiurge/element/om_demiurge_element_channel.hpp"
#include "openminecraft/renderer/common/demiurge/element/om_demiurge_element_cliprect_channel.hpp"
#include "openminecraft/renderer/common/demiurge/element/om_demiurge_element_roundedrect_channel.hpp"
#include "openminecraft/renderer/common/demiurge/element/om_demiurge_element_textsdf_channel.hpp"
#include "openminecraft/renderer/common/om_renderer_buffer.hpp"
#include "openminecraft/renderer/common/om_renderer_pipeline.hpp"
#include "openminecraft/renderer/common/om_renderer_shader.hpp"
#include "openminecraft/renderer/common/om_renderer_texture.hpp"
#include "openminecraft/renderer/common/wrap/om_renderer_temptarget.hpp"
#include "openminecraft/renderer/om_renderer_layer.hpp"
#include <array>
#include <chrono>
#include <memory>
#include <string>
#include <unordered_map>

namespace openminecraft::renderer::common::demiurge
{
OMDemiurgeRendererHandler::OMDemiurgeRendererHandler(OMRenderer *renderer, std::shared_ptr<OMDemiurgeNode> n)
    : OMRendererHandler(renderer), renderer(renderer), rect(renderer, [&]() -> void { recordTask(); }),
      roundedRect(renderer, [&]() -> void { recordTask(); }), image(renderer, [&]() -> void { recordTask(); }),
      sector(renderer, [&]() -> void { recordTask(); }), clipRect(renderer, [&]() -> void { recordTask(); }),
      svg(renderer, [&]() -> void { recordTask(); }), shaderScene(renderer, [&]() -> void { recordTask(); }),
      logger("OMDemiurgeRendererHandler", this)
{
    node = n;

    uniformBuffer = renderer->allocateBuffer(Uniform, sizeof(float) * 2);

    auto ext = renderer->getExtent();
    middleTarget = new wrap::OMRendererTempTarget(renderer);
    middleTarget->construct(ext);

    rect.init(uniformBuffer, middleTarget->target);
    roundedRect.init(uniformBuffer, middleTarget->target);
    image.init(uniformBuffer, middleTarget->target);
    sector.init(uniformBuffer, middleTarget->target);
    clipRect.init(uniformBuffer, middleTarget->target);
    svg.init(uniformBuffer, middleTarget->target);
    shaderScene.init(uniformBuffer, middleTarget->target);

    basics::OMVertexFormat simp;
    simp.nextGroup()->decideStruct();
}

OMDemiurgeRendererHandler::~OMDemiurgeRendererHandler()
{
    rect.destroy();
    roundedRect.destroy();
    image.destroy();
    sector.destroy();
    clipRect.destroy();
    svg.destroy();
    shaderScene.destroy();
    for (auto &p : fonts)
    {
        p.second->destroy();
    }
    delete uniformBuffer;

    delete middleTarget;
}

auto OMDemiurgeRendererHandler::fetchFontChannel(geom::OMFontSet *s)
    -> std::shared_ptr<element::OMDemiurgeTextSdfChannel>
{
    if (fonts.count(s))
    {
        return fonts[s];
    }

    auto c = std::make_shared<element::OMDemiurgeTextSdfChannel>(this->renderer, [&]() -> void { recordTask(); }, s);
    c->init(uniformBuffer, middleTarget->target);
    fonts[s] = c;
    return c;
}

void OMDemiurgeRendererHandler::submitTasks()
{
    middleTarget->construct(renderer->getExtent());

    auto ext = renderer->getLogicalExtent();
    uniformBuffer->updateData(std::array<float, 2>{ext.x, ext.y}.data());

    renderer->createTask("demiurgeui_compose_" + name);
    recordTask(true);
}

void OMDemiurgeRendererHandler::recordTask(bool resize)
{
    auto task = renderer->fetchTask("demiurgeui_compose_" + name);
    task->target(middleTarget->target);

    for (float layer = bottomDepth; layer >= topDepth; layer -= 0.01f)
    {
        rect.submitTask(task, layer + layerHalfWidth, layer - layerHalfWidth);
        roundedRect.submitTask(task, layer + layerHalfWidth, layer - layerHalfWidth);
        image.submitTask(task, layer + layerHalfWidth, layer - layerHalfWidth);
        sector.submitTask(task, layer + layerHalfWidth, layer - layerHalfWidth);
        clipRect.submitTask(task, layer + layerHalfWidth, layer - layerHalfWidth);
        svg.submitTask(task, layer + layerHalfWidth, layer - layerHalfWidth);
        shaderScene.submitTask(task, layer + layerHalfWidth, layer - layerHalfWidth);
        for (auto &p : fonts)
        {
            p.second->submitTask(task, layer + layerHalfWidth, layer - layerHalfWidth);
        }
    }
    task->finish();
    if (!resize)
    {
        renderer->taskRecreate("demiurgeui_compose_" + name);
    }
}

void OMDemiurgeRendererHandler::beforeFrame()
{
    auto ext = renderer->getLogicalExtent();

    node->layout(fit ? YGUndefined : ext.x, fit ? YGUndefined : ext.y);
    node->submit(this, bottomDepth);
    node->update();

    rect.update();
    roundedRect.update();
    image.update();
    sector.update();
    clipRect.update();
    svg.update();
    shaderScene.update();
    for (auto &p : fonts)
    {
        p.second->update();
    }
}

static void dumpTextureToPPM(OMRendererTexture *tex, const char *path)
{
    if (!tex)
    {
        return;
    }

    auto data = tex->fetchData(0);
    if (!data)
    {
        return;
    }

    const uint32_t w = static_cast<uint32_t>(tex->width);
    const uint32_t h = static_cast<uint32_t>(tex->height);
    const auto *px = static_cast<const uint8_t *>(data.get());

    const bool flipY = std::string(typeid(*tex).name()).find("OpenGL") != std::string::npos;

    std::ofstream out(path, std::ios::binary);
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

void OMDemiurgeRendererHandler::afterFrame()
{
    // 调试钩子：OM_DUMP_UI_TARGET=/path/ui.ppm（可选 OM_DUMP_UI_NAME=surface）导出一次渲染目标
    static const char *dumpPath = std::getenv("OM_DUMP_UI_TARGET");
    static const char *dumpName = std::getenv("OM_DUMP_UI_NAME");
    static bool dumped = false;
    static long long frame = 0;

    ++frame;

    static const long long dumpFrame = []() {
        const char *f = std::getenv("OM_DUMP_UI_FRAME");
        return f ? std::atoll(f) : 90LL;
    }();

    if (dumpPath && !dumped && frame >= dumpFrame && (!dumpName || name == dumpName))
    {
        dumped = true;
        dumpTextureToPPM(middleTarget ? middleTarget->colorTexture : nullptr, dumpPath);
        logger.info("demiurge target ({}) dumped to {}", name, dumpPath);
    }
}
} // namespace openminecraft::renderer::common::demiurge
