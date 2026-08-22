#include "openminecraft/renderer/common/demiurge/om_demiurge_rendererhandler.hpp"
#include "openminecraft/fontproc/om_fontset.hpp"
#include "openminecraft/renderer/common/demiurge/element/om_demiurge_element_channel.hpp"
#include "openminecraft/renderer/common/demiurge/element/om_demiurge_element_roundedrect_channel.hpp"
#include "openminecraft/renderer/common/demiurge/element/om_demiurge_element_textsdf_channel.hpp"
#include "openminecraft/renderer/common/om_renderer_buffer.hpp"
#include "openminecraft/renderer/common/om_renderer_pipeline.hpp"
#include "openminecraft/renderer/common/om_renderer_texture.hpp"
#include "openminecraft/renderer/common/wrap/om_renderer_temptarget.hpp"
#include "openminecraft/renderer/om_renderer_layer.hpp"
#include <memory>
#include <string>
#include <unordered_map>

namespace openminecraft::renderer::common::demiurge
{
struct SimpleUniform
{
    float width;
    float height;
};

OMDemiurgeRendererHandler::OMDemiurgeRendererHandler(OMRenderer *renderer, std::shared_ptr<OMDemiurgeNode> n)
    : OMRendererHandler(renderer), renderer(renderer), rect(renderer, [&]() -> void { recordTask(); }),
      roundedRect(renderer, [&]() -> void { recordTask(); }), image(renderer, [&]() -> void { recordTask(); }),
      sector(renderer, [&]() -> void { recordTask(); }), logger("OMDemiurgeRendererHandler", this)
{
    node = n;

    uniformBuffer = renderer->allocateBuffer(Uniform, sizeof(SimpleUniform));

    auto ext = renderer->getExtent();
    middleTarget = new wrap::OMRendererTempTarget(renderer);
    middleTarget->construct(ext);

    rect.init(uniformBuffer, middleTarget->target);
    roundedRect.init(uniformBuffer, middleTarget->target);
    image.init(uniformBuffer, middleTarget->target);
    sector.init(uniformBuffer, middleTarget->target);
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

    delete middleTarget;
}

auto OMDemiurgeRendererHandler::fetchFontChannel(fontproc::OMFontSet *s)
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
    SimpleUniform u{ext.x, ext.y};
    uniformBuffer->updateData(&u);

    renderer->createTask("demiurgeui_compose");
    recordTask(true);
}

void OMDemiurgeRendererHandler::recordTask(bool resize)
{
    auto task = renderer->fetchTask("demiurgeui_compose");
    task->target(middleTarget->target);

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
}
} // namespace openminecraft::renderer::common::demiurge
