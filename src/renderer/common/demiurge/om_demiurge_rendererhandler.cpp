#include "openminecraft/renderer/common/demiurge/om_demiurge_rendererhandler.hpp"
#include "glm/ext/vector_float4.hpp"
#include "openminecraft/renderer/common/demiurge/element/om_demiurge_element_channel.hpp"
#include "openminecraft/renderer/common/demiurge/element/om_demiurge_element_roundedrect_channel.hpp"
#include "openminecraft/renderer/common/demiurge/node/om_demiurge_container.hpp"
#include "openminecraft/renderer/common/demiurge/node/om_demiurge_image.hpp"
#include "openminecraft/renderer/common/demiurge/node/om_demiurge_rect.hpp"
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
    : renderer(renderer), OMRendererHandler(renderer), rect(renderer), roundedRect(renderer), image(renderer),
      logger("OMDemiurgeRendererHandler", this)
{
    {
        auto imgraw = vfs::fsfetch("/bootassets/openminecraft-renderer/texture/viking_room.png");

        specs::png::OMPngFile img;
        img.parse(imgraw);

        texture = renderer->allocateTexture(img.getWidth(), img.getHeight(), Dim2, ColorRgba);
        texture->updateData(img.fetchData());
    }

    node = std::make_shared<node::OMDemiurgeImageNode>(texture)->style({
        {"color", (int)0xffffffff},
        {"flexGap", 10_px},
        {"flexWrap", Wrap},
        {"flexDirection", Row},
        {"fill", node::OMDemiurgeImageFillType::Contain},
    });

    for (int i = 0; i < 3; ++i)
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
            auto d2 = std::make_shared<node::OMDemiurgeRectNode>()->style({
                {"color", a[i]},
                {"minWidth", 1.0f},
                {"minHeight", 1.0f},
                {"flexGrow", 1.0f},
                {"radius", glm::vec4(30, 30, 30, 30)},
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
    delete uniformBuffer;
    delete texture;
}

void OMDemiurgeRendererHandler::submitTasks()
{
    srand(time(nullptr));
    auto ext = renderer->getExtent();
    SimpleUniform u{ext.x, ext.y};
    uniformBuffer->updateData(&u);

    auto task = renderer->createTask()
                    ->dependOn(renderer->fetchTask("main"))
                    ->target(renderer->getDefaultRenderTarget())
                    ->clearN();

    element::OMDemiurgeAbstractChannel *channel = nullptr;
    for (float layer = bottomDepth; layer >= topDepth; layer -= 0.01f)
    {
        rect.submitTask(task, layer + layerHalfWidth, layer - layerHalfWidth, channel);
        roundedRect.submitTask(task, layer + layerHalfWidth, layer - layerHalfWidth, channel);
        image.submitTask(task, layer + layerHalfWidth, layer - layerHalfWidth, channel);
    }
    task->finish();

    renderer->registerTask("demiurgeui_compose", task);
}

void OMDemiurgeRendererHandler::beforeFrame()
{
    auto ext = renderer->getExtent();

    node->layout(ext.x, ext.y);
    node->submit(this, bottomDepth);

    rect.update();
    roundedRect.update();
    image.update();
}

void OMDemiurgeRendererHandler::afterFrame()
{
}
} // namespace openminecraft::renderer::common::demiurge
