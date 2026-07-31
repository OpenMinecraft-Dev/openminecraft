#include "openminecraft/renderer/common/demiurge/om_demiurge_rendererhandler.hpp"
#include "openminecraft/fontproc/om_fontset.hpp"
#include "openminecraft/renderer/common/demiurge/element/om_demiurge_element_channel.hpp"
#include "openminecraft/renderer/common/demiurge/element/om_demiurge_element_roundedrect_channel.hpp"
#include "openminecraft/renderer/common/demiurge/element/om_demiurge_element_textsdf_channel.hpp"
#include "openminecraft/renderer/common/demiurge/node/om_demiurge_container.hpp"
#include "openminecraft/renderer/common/demiurge/node/om_demiurge_image.hpp"
#include "openminecraft/renderer/common/demiurge/node/om_demiurge_textsdf.hpp"
#include "openminecraft/renderer/common/demiurge/om_demiurge_geometry.hpp"
#include "openminecraft/renderer/common/om_renderer_buffer.hpp"
#include "openminecraft/renderer/common/om_renderer_pipeline.hpp"
#include "openminecraft/renderer/common/om_renderer_texture.hpp"
#include "openminecraft/renderer/om_renderer_layer.hpp"
#include "openminecraft/specs/png/om_png.hpp"
#include "openminecraft/vfs/om_vfs_base.hpp"
#include <memory>
#include <array>

namespace openminecraft::renderer::common::demiurge
{
struct SimpleUniform
{
    float width;
    float height;
};
constexpr std::array<int, 3> a = {0x00d4ffaa, (int)0xbfff00aa, (int)0xff4500aa};
OMDemiurgeRendererHandler::OMDemiurgeRendererHandler(OMRenderer *renderer)
    : renderer(renderer), OMRendererHandler(renderer), rect(renderer, [&]() -> void { recordTask(); }),
      roundedRect(renderer, [&]() -> void { recordTask(); }), image(renderer, [&]() -> void { recordTask(); }),
      logger("OMDemiurgeRendererHandler", this)
{
    {
        auto imgraw = vfs::fsfetch("/bootassets/openminecraft-renderer/texture/viking_room.png");

        specs::png::OMPngFile img;
        img.parse(imgraw);

        texture = renderer->allocateTexture(img.getWidth(), img.getHeight(), Dim2, ColorRgba);
        // texture->updateData(img.fetchData());
    }

    fontset = std::make_shared<fontproc::OMFontSet>();
    auto rawfile = vfs::fsfetch("/bootassets/openminecraft-boot/font/MapleMono-NF-Regular.ttf");
    fontset->fontList.push_back(std::make_shared<fontproc::OMFont>(*rawfile.get()));

    node = std::make_shared<node::OMDemiurgeImageNode>(texture)->style({
        {"color", (int)0xffffffff},
        {"flexGap", 10_px},
        {"flexWrap", Wrap},
        {"flexDirection", Row},
        {"fill", node::OMDemiurgeImageFillType::Contain},
    });

    for (int i = 0; i < 1; ++i)
    {
        auto d = std::make_shared<node::OMDemiurgeContainerNode>()->style({
            {"minWidth", 100.0f},
            {"minHeight", 100.0f},
            {"height", 100_percent},
            {"flexShrink", 0.0f},
            {"flexGrow", 1.0f},
            {"flexGap", 10_px},
        });

        for (int j = 0; j < 6; ++j)
        {
            auto d2 = std::make_shared<node::OMDemiurgeTextSdfNode>(fontset.get())
                          ->style({
                              {"color", a[i]},
                              {"minWidth", 1.0f},
                              {"minHeight", 1.0f},
                              {"flexGrow", 1.0f},
                              {"text", std::string("The quick brown fox jumps over the lazy dog.-> <- && ===")},
                              {"textheight", 24},
                          });
            d->mount(d2);
        }
        node->mount(d);
        target = d;
    }

    uniformBuffer = renderer->allocateBuffer(Uniform, sizeof(SimpleUniform));

    rect.init(uniformBuffer, renderer->getDefaultRenderTarget());
    roundedRect.init(uniformBuffer, renderer->getDefaultRenderTarget());
    image.init(uniformBuffer, renderer->getDefaultRenderTarget());
}

OMDemiurgeRendererHandler::~OMDemiurgeRendererHandler()
{
    rect.destroy();
    roundedRect.destroy();
    image.destroy();
    for (auto &p : fonts)
    {
        p.second->destroy();
    }
    delete uniformBuffer;
    delete texture;
}

auto OMDemiurgeRendererHandler::fetchFontChannel(fontproc::OMFontSet *s)
    -> std::shared_ptr<element::OMDemiurgeTextSdfChannel>
{
    if (fonts.count(s))
    {
        return fonts[s];
    }

    auto c = std::make_shared<element::OMDemiurgeTextSdfChannel>(this->renderer, [&]() -> void { recordTask(); }, s);
    c->init(uniformBuffer, renderer->getDefaultRenderTarget());
    fonts[s] = c;
    return c;
}

void OMDemiurgeRendererHandler::submitTasks()
{
    srand(time(nullptr));
    auto ext = renderer->getExtent();
    SimpleUniform u{ext.x, ext.y};
    uniformBuffer->updateData(&u);

    this->task = renderer->createTask();
    renderer->registerTask("demiurgeui_compose", task);
    recordTask(true);
}

void OMDemiurgeRendererHandler::recordTask(bool resize)
{
    task->dependOn(renderer->fetchTask("main"))->target(renderer->getDefaultRenderTarget())->clearN();

    element::OMDemiurgeAbstractChannel *channel = nullptr;
    for (float layer = bottomDepth; layer >= topDepth; layer -= 0.01f)
    {
        rect.submitTask(task, layer + layerHalfWidth, layer - layerHalfWidth, channel);
        roundedRect.submitTask(task, layer + layerHalfWidth, layer - layerHalfWidth, channel);
        image.submitTask(task, layer + layerHalfWidth, layer - layerHalfWidth, channel);
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

void OMDemiurgeRendererHandler::beforeFrame()
{
    auto ext = renderer->getExtent();

    node->layout(ext.x, ext.y);
    node->submit(this, bottomDepth);

    rect.update();
    roundedRect.update();
    image.update();
    for (auto &p : fonts)
    {
        p.second->update();
    }
}

void OMDemiurgeRendererHandler::afterFrame()
{
}
} // namespace openminecraft::renderer::common::demiurge
