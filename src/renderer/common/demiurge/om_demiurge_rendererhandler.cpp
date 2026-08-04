#include "openminecraft/renderer/common/demiurge/om_demiurge_rendererhandler.hpp"
#include "glm/ext/vector_float4.hpp"
#include "glm/fwd.hpp"
#include "openminecraft/renderer/common/demiurge/node/om_demiurge_sector.hpp"
#include "openminecraft/fontproc/om_fontset.hpp"
#include "openminecraft/renderer/common/demiurge/element/om_demiurge_element_channel.hpp"
#include "openminecraft/renderer/common/demiurge/element/om_demiurge_element_roundedrect_channel.hpp"
#include "openminecraft/renderer/common/demiurge/element/om_demiurge_element_textsdf_channel.hpp"
#include "openminecraft/renderer/common/demiurge/node/om_demiurge_container.hpp"
#include "openminecraft/renderer/common/demiurge/node/om_demiurge_image.hpp"
#include "openminecraft/renderer/common/demiurge/node/om_demiurge_rect.hpp"
#include "openminecraft/renderer/common/demiurge/node/om_demiurge_sector.hpp"
#include "openminecraft/renderer/common/demiurge/node/om_demiurge_textsdf.hpp"
#include "openminecraft/renderer/common/demiurge/om_demiurge_geometry.hpp"
#include "openminecraft/renderer/common/om_renderer_buffer.hpp"
#include "openminecraft/renderer/common/om_renderer_pipeline.hpp"
#include "openminecraft/renderer/common/om_renderer_texture.hpp"
#include "openminecraft/renderer/om_renderer_layer.hpp"
#include "openminecraft/specs/png/om_png.hpp"
#include "openminecraft/vfs/om_vfs_base.hpp"
#include <cstdint>
#include <cstdlib>
#include <map>
#include <memory>
#include <array>
#include <chrono>
#include <string>
#include <unordered_map>
#include <vector>

namespace openminecraft::renderer::common::demiurge
{
struct SimpleUniform
{
    float width;
    float height;
};
constexpr std::array<int, 3> a = {0x00d4ffff, (int)0xbfff00ff, (int)0xff4500ff};
constexpr std::array<int, 16> DebugColors = {
    (int)0xF9FFFFFF, (int)0xF9801DFF, (int)0xC74EBDFF, 0x3AB3DAFF, (int)0xFED83DFF, (int)0x80C71FFF,
    (int)0xF38BAAFF, 0x474F52FF,      (int)0x9D9D97FF, 0x169C9CFF, (int)0x8932B8FF, 0x3C44AAFF,
    (int)0x835432FF, 0x5E7C16FF,      (int)0xB02E26FF, 0x1D1D21FF,
};
OMDemiurgeRendererHandler::OMDemiurgeRendererHandler(OMRenderer *renderer)
    : renderer(renderer), OMRendererHandler(renderer), rect(renderer, [&]() -> void { recordTask(); }),
      roundedRect(renderer, [&]() -> void { recordTask(); }), image(renderer, [&]() -> void { recordTask(); }),
      sector(renderer, [&]() -> void { recordTask(); }), logger("OMDemiurgeRendererHandler", this)
{
    {
        auto imgraw = vfs::fsfetch("/bootassets/openminecraft-renderer/texture/summer_1am.png");

        specs::png::OMPngFile img;
        img.parse(imgraw);

        texture = renderer->allocateTexture(img.getWidth(), img.getHeight(), Dim2, ColorRgba);
        texture->updateData(img.fetchData());
    }

    fontset = std::make_shared<fontproc::OMFontSet>();
    auto rawfile = vfs::fsfetch("/bootassets/openminecraft-boot/font/MapleMono-NF-Regular.ttf");
    fontset->fontList.push_back(std::make_shared<fontproc::OMFont>(*rawfile.get()));
    auto rawfile2 = vfs::fsfetch("/bootassets/openminecraft-boot/font/StarRailFont.ttf");
    fontset->fontList.push_back(std::make_shared<fontproc::OMFont>(*rawfile2.get()));
    auto rawfile3 = vfs::fsfetch("/bootassets/openminecraft-boot/font/NotoSansArabic.ttf");
    fontset->fontList.push_back(std::make_shared<fontproc::OMFont>(*rawfile3.get()));

    node = std::make_shared<node::OMDemiurgeImageNode>(texture)
               ->style({
                   {"color", (int)0xffffffff},
                   {"flexGap", 10_px},
                   {"flexWrap", Wrap},
                   {"flexDirection", Column},
                   {"fill", node::OMDemiurgeImageFillType::Cover},
                   {"radius", glm::vec4(25.0f)},
               })
               ->mount(std::make_shared<node::OMDemiurgeContainerNode>()
                           ->style({
                               {"flexShrink", 0.0f},
                               {"flexGrow", 0.0f},
                               {"flexDirection", Column},
                               {"width", 100_percent},
                           })
                           ->mount(std::make_shared<node::OMDemiurgeTextSdfNode>(fontset.get())
                                       ->style({
                                           {"color", (int)0xffffffff},
                                           {"flexGrow", 1.0f},
                                           {"text", "OpenMinecraft Debug Screen"},
                                           {"textheight", 24},
                                       }))
                           ->mount(std::make_shared<node::OMDemiurgeTextSdfNode>(fontset.get())
                                       ->style({
                                           {"color", (int)0xffffffff},
                                           {"flexGrow", 1.0f},
                                           {"text", ""},
                                           {"textheight", 24},
                                       })
                                       ->store(fpsTextNode)))
               ->mount(std::make_shared<node::OMDemiurgeRectNode>()
                           ->style({
                               {"flexShrink", 0.0f},
                               {"flexGrow", 1.0f},
                               {"width", 100_percent},
                               {"color", 0x222222ff},
                               {"radius", glm::vec4(25.0f)},
                           })
                           ->mount(std::make_shared<node::OMDemiurgeContainerNode>()
                                       ->style({
                                           {"flexShrink", 0.0f},
                                           {"flexGrow", 1.0f},
                                       })
                                       ->store(graphNode))
                           ->store(textNode));

    uniformBuffer = renderer->allocateBuffer(Uniform, sizeof(SimpleUniform));

    auto ext = renderer->getExtent();
    middleTexture = renderer->allocateTexture(ext.x, ext.y, Dim2, ColorRgba);
    middleDepth = renderer->allocateTexture(ext.x, ext.y, Dim2, Depth);

    middleRenderTarget = renderer->createRenderTarget();
    middleRenderTarget->attachTarget(middleTexture);
    middleRenderTarget->attachTarget(middleDepth);
    middleRenderTarget->build();

    rect.init(uniformBuffer, middleRenderTarget);
    roundedRect.init(uniformBuffer, middleRenderTarget);
    image.init(uniformBuffer, middleRenderTarget);
    sector.init(uniformBuffer, middleRenderTarget);

    srand(time(nullptr));
}

OMDemiurgeRendererHandler::~OMDemiurgeRendererHandler()
{
    rect.destroy();
    roundedRect.destroy();
    image.destroy();
    sector.destroy();
    for (auto &p : fonts)
    {
        p.second->destroy();
    }
    delete uniformBuffer;
    delete texture;

    delete middleTexture;
    delete middleDepth;
    delete middleRenderTarget;
}

auto OMDemiurgeRendererHandler::updateState(util::OMTicker &t) -> void
{
    std::map<std::string, uint64_t> results;
    uint64_t total = 0;
    for (auto &e : t.ticks)
    {
        if (e.first.depth != 1)
        {
            continue;
        }

        if (e.first.pop)
        {
            results[e.first.id] = e.second - results[e.first.id];
            total += results[e.first.id];
        }
        else
        {
            results[e.first.id] = e.second;
        }
    }

    while (sectorNodes.size() != results.size())
    {
        if (sectorNodes.size() > results.size())
        {
            graphNode->umount(sectorNodes.back());
            textNode->umount(textNodes.back());
            sectorNodes.pop_back();
            textNodes.pop_back();
        }
        else
        {
            auto n = std::make_shared<node::OMDemiurgeSectorNode>()->style({
                {"position", OMDemiurgePosition::Absolute},
                {"width", 100_percent},
                {"height", 100_percent},
                {"rotationPivot", glm::vec3(0.0f, -2.0f, 1.0f)},
            });
            graphNode->mount(n);
            sectorNodes.emplace_back(n);

            auto n2 = std::make_shared<node::OMDemiurgeTextSdfNode>(fontset.get())
                          ->style({
                              {"flexGrow", 0.0f},
                              {"textheight", 24},
                          });
            textNode->mount(n2);
            textNodes.emplace_back(n2);
        }
    }

    float curang = glm::radians(90.0f);
    int i = 0;
    std::vector<std::pair<std::string, uint64_t>> vec(results.begin(), results.end());

    std::sort(vec.begin(), vec.end(), [](const auto &a, const auto &b) -> auto { return a.second > b.second; });
    for (auto &p : vec)
    {
        sectorNodes[i]->style("beginAngle", curang)->style("color", DebugColors[i % 16]);
        auto fct = (double)p.second / (double)total;
        curang += glm::radians(360.0f) * fct;
        sectorNodes[i]->style("endAngle", curang);
        textNodes[i]
            ->style("text",
                    fmt::format("{}: {:.2f} us / {:.2f} %", p.first, static_cast<float>(p.second) / 1000, fct * 100))
            ->style("color", DebugColors[i % 16]);
        ++i;
    }
}

auto OMDemiurgeRendererHandler::fetchFontChannel(fontproc::OMFontSet *s)
    -> std::shared_ptr<element::OMDemiurgeTextSdfChannel>
{
    if (fonts.count(s))
    {
        return fonts[s];
    }

    auto c = std::make_shared<element::OMDemiurgeTextSdfChannel>(this->renderer, [&]() -> void { recordTask(); }, s);
    c->init(uniformBuffer, middleRenderTarget);
    fonts[s] = c;
    return c;
}

void OMDemiurgeRendererHandler::submitTasks()
{
    {
        delete middleDepth;
        delete middleTexture;

        auto ext = renderer->getExtent();
        middleTexture = renderer->allocateTexture(ext.x, ext.y, Dim2, ColorRgba);
        middleDepth = renderer->allocateTexture(ext.x, ext.y, Dim2, Depth);

        middleRenderTarget->replaceTarget(0, middleTexture);
        middleRenderTarget->replaceTarget(1, middleDepth);
        middleRenderTarget->rebuild();
    }

    srand(time(nullptr));
    auto ext = renderer->getLogicalExtent();
    SimpleUniform u{ext.x, ext.y};
    uniformBuffer->updateData(&u);

    this->task = renderer->createTask();
    renderer->registerTask("demiurgeui_compose", task);
    recordTask(true);
}

void OMDemiurgeRendererHandler::recordTask(bool resize)
{
    task->dependOn(renderer->fetchTask("main"))->target(middleRenderTarget)->clearN();

    element::OMDemiurgeAbstractChannel *channel = nullptr;
    for (float layer = bottomDepth; layer >= topDepth; layer -= 0.01f)
    {
        rect.submitTask(task, layer + layerHalfWidth, layer - layerHalfWidth, channel);
        roundedRect.submitTask(task, layer + layerHalfWidth, layer - layerHalfWidth, channel);
        image.submitTask(task, layer + layerHalfWidth, layer - layerHalfWidth, channel);
        sector.submitTask(task, layer + layerHalfWidth, layer - layerHalfWidth, channel);
        for (auto &p : fonts)
        {
            p.second->submitTask(task, layer + layerHalfWidth, layer - layerHalfWidth, channel);
        }
    }
    task->finish();
    if (!resize)
    {
        renderer->taskRecreate("demiurgeui_compose");
    }
}

static std::chrono::steady_clock::time_point tp = {};

void OMDemiurgeRendererHandler::beforeFrame()
{
    auto ext = renderer->getLogicalExtent();

    node->layout(ext.x, ext.y);
    node->submit(this, bottomDepth);

    rect.update();
    roundedRect.update();
    image.update();
    sector.update();
    for (auto &p : fonts)
    {
        p.second->update();
    }
}

void OMDemiurgeRendererHandler::afterFrame()
{
    ++fps;

    if (tp == std::chrono::steady_clock::time_point{})
    {
        tp = std::chrono::steady_clock::now();
        return;
    }

    auto tpe = std::chrono::steady_clock::now();
    auto cc = std::chrono::duration_cast<std::chrono::nanoseconds>(tpe - tp);
    if (cc.count() > 1e8)
    {
        fpsTextNode->style("text",
                           fmt::format("FPS: {}", static_cast<int>(static_cast<float>(fps) / cc.count() * 1e9)));

        tp = std::chrono::steady_clock::now();
        fps = 0;
    }
}
} // namespace openminecraft::renderer::common::demiurge
