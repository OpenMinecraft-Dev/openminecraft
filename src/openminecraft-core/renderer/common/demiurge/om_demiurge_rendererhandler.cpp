#include "openminecraft/renderer/common/demiurge/om_demiurge_rendererhandler.hpp"
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
      svg(renderer, [&]() -> void { recordTask(); }), logger("OMDemiurgeRendererHandler", this)
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
    for (auto &p : fonts)
    {
        p.second->update();
    }
}

void OMDemiurgeRendererHandler::afterFrame()
{
}
} // namespace openminecraft::renderer::common::demiurge
