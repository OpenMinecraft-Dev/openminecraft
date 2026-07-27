#include "openminecraft/renderer/common/demiurge/om_demiurge_rendererhandler.hpp"
#include "glm/ext/vector_float4.hpp"
#include "openminecraft/renderer/common/demiurge/element/om_demiurge_element_channel.hpp"
#include "openminecraft/renderer/common/demiurge/element/om_demiurge_element_roundedrect_channel.hpp"
#include "openminecraft/renderer/common/demiurge/node/om_demiurge_rect.hpp"
#include "openminecraft/renderer/common/demiurge/om_demiurge_geometry.hpp"
#include "openminecraft/renderer/common/om_renderer_buffer.hpp"
#include "openminecraft/renderer/common/om_renderer_pipeline.hpp"
#include "openminecraft/renderer/common/om_renderer_texture.hpp"
#include "openminecraft/renderer/om_renderer_layer.hpp"
#include <iostream>
#include <memory>

namespace openminecraft::renderer::common::demiurge
{
struct SimpleUniform
{
    float width;
    float height;
};
int a[] = {0x00d4ffff, (int)0xbfff00ff, (int)0xff4500ff};
OMDemiurgeRendererHandler::OMDemiurgeRendererHandler(OMRenderer *renderer)
    : renderer(renderer), OMRendererHandler(renderer), rect(renderer), roundedRect(renderer),
      logger("OMDemiurgeRendererHandler", this)
{
    node = std::make_shared<node::OMDemiurgeRectNode>()->style({
        {"color", 0x2c2c34ff},
        {"flexGap", 10_px},
        {"flexWrap", Wrap},
        {"flexDirection", Row},
    });

    for (int i = 0; i < 3; ++i)
    {
        auto d = std::make_shared<node::OMDemiurgeRectNode>()->style({
            {"color", 0},
            {"minWidth", 100.0f},
            {"minHeight", 100.0f},
            {"height", 100_percent},
            {"flexShrink", 0.0f},
            {"flexGrow", 1.0f},
            {"radius", glm::vec4(30, 30, 30, 30)},
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
}

OMDemiurgeRendererHandler::~OMDemiurgeRendererHandler()
{
    rect.destroy();
    roundedRect.destroy();
    delete uniformBuffer;
}

void OMDemiurgeRendererHandler::submitTasks()
{
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
}

bool um = true;

void OMDemiurgeRendererHandler::afterFrame()
{
    if (um)
    {
        node->umount(target);
    }
    else
    {
        node->mount(target);
    }

    um = !um;

    std::cout << (um ? "mount" : "umount") << std::endl;
}
} // namespace openminecraft::renderer::common::demiurge
