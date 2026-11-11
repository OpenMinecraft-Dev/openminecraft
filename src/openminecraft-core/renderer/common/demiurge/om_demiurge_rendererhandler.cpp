#include "openminecraft/renderer/common/demiurge/om_demiurge_rendererhandler.hpp"
#include "openminecraft/renderer/common/demiurge/om_demiurge_layer.hpp"
#include <vector>
#include <typeinfo>
#include "openminecraft/geom/om_fontset.hpp"
#include "openminecraft/geom/om_svg_structure.hpp"
#include "openminecraft/renderer/common/basics/om_vertex_format.hpp"
#include "openminecraft/renderer/common/demiurge/element/om_demiurge_element_channel.hpp"
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
      sector(renderer, [&]() -> void { recordTask(); }),
      svg(renderer, [&]() -> void { recordTask(); }), shaderScene(renderer, [&]() -> void { recordTask(); }),
      logger("OMDemiurgeRendererHandler", this)
{
    node = n;

    uniformBuffer = wrap::OMBufferRef(renderer, "demiurge.uniform." + std::to_string(wrap::nextObjectTagId()));
    renderer->registerBuffer(uniformBuffer.getTag(), Uniform, sizeof(float) * 2);

    auto ext = renderer->getExtent();
    middleTarget = new wrap::OMRendererTempTarget(renderer, "demiurge.middle");
    middleTarget->construct(ext);

    rect.init(uniformBuffer.getTag(), middleTarget->targetTagName());
    roundedRect.init(uniformBuffer.getTag(), middleTarget->targetTagName());
    image.init(uniformBuffer.getTag(), middleTarget->targetTagName());
    sector.init(uniformBuffer.getTag(), middleTarget->targetTagName());
    svg.init(uniformBuffer.getTag(), middleTarget->targetTagName());
    shaderScene.init(uniformBuffer.getTag(), middleTarget->targetTagName());

    basics::OMVertexFormat simp;
    simp.nextGroup()->decideStruct();
}

OMDemiurgeRendererHandler::~OMDemiurgeRendererHandler()
{
    rect.destroy();
    roundedRect.destroy();
    image.destroy();
    sector.destroy();
    svg.destroy();
    shaderScene.destroy();
    for (auto &p : fonts)
    {
        p.second->destroy();
    }

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
    c->init(uniformBuffer.getTag(), middleTarget->targetTagName());
    fonts[s] = c;
    return c;
}

void OMDemiurgeRendererHandler::submitTasks()
{
    middleTarget->construct(renderer->getExtent());

    auto ext = renderer->getLogicalExtent() / renderer->getUiScale();
    uniformBuffer->updateData(std::array<float, 2>{ext.x, ext.y}.data());

    renderer->createTask("demiurgeui_compose_" + name);
    recordTask(true);
}

void OMDemiurgeRendererHandler::recordTask(bool resize)
{
    auto task = renderer->fetchTask("demiurgeui_compose_" + name);
    task->target(middleTarget->target());

    uint32_t maxLayer = rect.maxLayer();
    uint32_t maxOverlayLayer = rect.maxLayer(true);
    bool hasOverlay = rect.hasLayerGroup(true);

    auto includeMax = [&](auto &channel) {
        maxLayer = std::max(maxLayer, channel.maxLayer());
        maxOverlayLayer = std::max(maxOverlayLayer, channel.maxLayer(true));
        hasOverlay = hasOverlay || channel.hasLayerGroup(true);
    };
    includeMax(roundedRect);
    includeMax(image);
    includeMax(sector);
    includeMax(svg);
    includeMax(shaderScene);

    for (auto &p : fonts)
    {
        includeMax(*p.second);
    }

    // Normal layers first.
    for (uint32_t layer = 0; layer <= maxLayer; ++layer)
    {
        rect.submitTask(task, layer);
        roundedRect.submitTask(task, layer);
        image.submitTask(task, layer);
        sector.submitTask(task, layer);
        svg.submitTask(task, layer);
        shaderScene.submitTask(task, layer);

        for (auto &p : fonts)
        {
            p.second->submitTask(task, layer);
        }
    }

    // Overlay group: highest bit set, separate max, always after normal layers.
    if (hasOverlay)
    {
        for (uint32_t layer = 0; layer <= maxOverlayLayer; ++layer)
        {
            const uint32_t overlay = overlayLayer(layer);

            rect.submitTask(task, overlay);
            roundedRect.submitTask(task, overlay);
            image.submitTask(task, overlay);
            sector.submitTask(task, overlay);
            svg.submitTask(task, overlay);
            shaderScene.submitTask(task, overlay);

            for (auto &p : fonts)
            {
                p.second->submitTask(task, overlay);
            }
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
    auto ext = renderer->getLogicalExtent() / renderer->getUiScale();

    node->flushDeferredActions();
    node->layout(fit ? YGUndefined : ext.x, fit ? YGUndefined : ext.y);
    node->submit(this, 0);
    node->update();
    node->flushDeferredActions();

    rect.update();
    roundedRect.update();
    image.update();
    sector.update();
    svg.update();
    shaderScene.update();
    for (auto &p : fonts)
    {
        p.second->update();
    }

}

void OMDemiurgeRendererHandler::afterFrame()
{
    // Scissor commands are baked into the recorded task; re-record when a clip
    // rectangle changed (layout moved) so the clip follows the node.
    if (clipDirty)
    {
        clipDirty = false;
        recordTask();
    }
}
} // namespace openminecraft::renderer::common::demiurge
