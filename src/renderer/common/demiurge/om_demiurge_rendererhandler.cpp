#include "openminecraft/renderer/common/demiurge/om_demiurge_rendererhandler.hpp"
#include "openminecraft/renderer/common/demiurge/element/om_demiurge_element_roundedrect_channel.hpp"
#include "openminecraft/renderer/common/demiurge/node/om_demiurge_rect.hpp"
#include "openminecraft/renderer/common/demiurge/om_demiurge_geometry.hpp"
#include "openminecraft/renderer/common/om_renderer_buffer.hpp"
#include "openminecraft/renderer/common/om_renderer_pipeline.hpp"
#include "openminecraft/renderer/common/om_renderer_texture.hpp"
#include "openminecraft/renderer/om_renderer_layer.hpp"
#include <memory>

namespace openminecraft::renderer::common::demiurge
{
struct SimpleUniform
{
    float width;
    float height;
};
OMDemiurgeRendererHandler::OMDemiurgeRendererHandler(OMRenderer *renderer)
    : renderer(renderer), OMRendererHandler(renderer), rect(renderer), roundedRect(renderer)
{
    // 0x2c2c34ff 0x00d4ffff
    node = std::make_shared<node::OMDemiurgeRectNode>()->style({
        {"color", 0x2c2c34ff},
        {"flexGap", 10_px},
        {"flexWrap", Wrap},
        {"flexDirection", Row},
    });
    for (int i = 0; i < 4; ++i)
    {
        auto d = std::make_shared<node::OMDemiurgeRectNode>()->style({
            {"color", 0x00d4ffff},
            {"minWidth", 100.0f},
            {"minHeight", 100.0f},
            {"height", 50_percent},
            {"flexShrink", 0.0f},
            {"flexGrow", 1.0f},
        });
        node->mount(d);
    }
    uniformBuffer = renderer->allocateBuffer(Uniform, sizeof(SimpleUniform));
}

OMDemiurgeRendererHandler::~OMDemiurgeRendererHandler()
{
    rect.destroy();
    roundedRect.destroy();
    delete uniformBuffer;

    delete composeTarget;
    delete composeColor;
    delete composeDepth;
}

void OMDemiurgeRendererHandler::submitTasks()
{
    auto ext = renderer->getExtent();
    SimpleUniform u{ext.x, ext.y};
    uniformBuffer->updateData(&u);

    {
        if (composeTarget)
        {
            delete composeColor;
            delete composeDepth;
        }

        composeColor = renderer->allocateTexture(ext.x, ext.y, Dim2, ColorRgba);
        composeDepth = renderer->allocateTexture(ext.x, ext.y, Dim2, Depth);

        if (!composeTarget)
        {
            composeTarget = renderer->createRenderTarget();
            composeTarget->attachTarget(composeColor);
            composeTarget->attachTarget(composeDepth);
            composeTarget->build();

            rect.init(uniformBuffer, composeTarget);
            roundedRect.init(uniformBuffer, composeTarget);
        }
        else
        {
            composeTarget->replaceTarget(0, composeColor);
            composeTarget->replaceTarget(1, composeDepth);
            composeTarget->rebuild();
        }
    }

    auto task = renderer->createTask()->dependOn(renderer->fetchTask("main"))->target(composeTarget)->clearN();
    rect.submitTask(task);
    roundedRect.submitTask(task);
    task->finish();

    renderer->registerTask("demiurgeui_compose", task);
}

void OMDemiurgeRendererHandler::beforeFrame()
{
    auto ext = renderer->getExtent();

    node->layout(ext.x, ext.y);
    node->submit(this, 0.9f);

    rect.update();
    roundedRect.update();
}

void OMDemiurgeRendererHandler::afterFrame()
{
}
} // namespace openminecraft::renderer::common::demiurge
