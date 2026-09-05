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
      sector(renderer, [&]() -> void { recordTask(); }), clipRect(renderer, [&]() -> void { recordTask(); }),
      logger("OMDemiurgeRendererHandler", this)
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
    clipRect.init(uniformBuffer, middleTarget->target);

    auto comp =
        openminecraft::geom::svg::compile(openminecraft::geom::svg::mergeTo(openminecraft::geom::svg::parseSvgPath(
                                              "M4.153 7.326c-.099.272.042.578.327.629a3 3 0 1 "
                                              "0-2.438-2.456c.048.286.353.428.626.331s.408-.399.387-.688a1.95 1.95 0 1 "
                                              "1 1.79 1.802c-.29-.023-.592.11-.692.382")),
                                          {10.0f, 10.0f});
    testBuffer = renderer->allocateBuffer(UniformTexel, 4 * comp.size());
    testBuffer->updateData(comp.data());

    basics::OMVertexFormat simp;
    simp.nextGroup()->decideStruct();

    testPipeline =
        renderer->createPipeline()
            ->input(UniformTexelBuffer)
            ->inputName("inSvgData")
            ->output(middleTarget->target)
            ->shader(renderer->shaderManager.preprocess("demiurge/svg.frag.glsl", Fragment, GLSLSource, simp))
            ->shader(renderer->shaderManager.preprocess("demiurge/svg.vert.glsl", Vertex, GLSLSource, simp))
            ->format(simp)
            ->blendFunc({SrcAlpha, OneMinusSrcAlpha, One, OneMinusSrcAlpha})
            ->blend(true)
            ->depth(true, true)
            ->buildN();
    testPipeline->bindInput(0, testBuffer);
}

OMDemiurgeRendererHandler::~OMDemiurgeRendererHandler()
{
    delete testBuffer;
    delete testPipeline;

    rect.destroy();
    roundedRect.destroy();
    image.destroy();
    sector.destroy();
    clipRect.destroy();
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
    SimpleUniform u{ext.x, ext.y};
    uniformBuffer->updateData(&u);

    renderer->createTask("demiurgeui_compose");
    recordTask(true);
}

void OMDemiurgeRendererHandler::recordTask(bool resize)
{
    auto task = renderer->fetchTask("demiurgeui_compose");
    task->target(middleTarget->target);

    for (float layer = bottomDepth; layer >= topDepth; layer -= 0.01f)
    {
        clipRect.submitTask(task, layer + layerHalfWidth, layer - layerHalfWidth);
        rect.submitTask(task, layer + layerHalfWidth, layer - layerHalfWidth);
        roundedRect.submitTask(task, layer + layerHalfWidth, layer - layerHalfWidth);
        image.submitTask(task, layer + layerHalfWidth, layer - layerHalfWidth);
        sector.submitTask(task, layer + layerHalfWidth, layer - layerHalfWidth);
        for (auto &p : fonts)
        {
            p.second->submitTask(task, layer + layerHalfWidth, layer - layerHalfWidth);
        }
    }
    task->pipeline(testPipeline)->drawN(6);
    task->finish();
    if (!resize)
    {
        renderer->taskRecreate("demiurgeui_compose");
    }
}

void OMDemiurgeRendererHandler::beforeFrame()
{
    auto ext = renderer->getLogicalExtent();

    node->layout(fit ? YGUndefined : ext.x, fit ? YGUndefined : ext.y);
    node->submit(this, bottomDepth);

    rect.update();
    roundedRect.update();
    image.update();
    sector.update();
    clipRect.update();
    for (auto &p : fonts)
    {
        p.second->update();
    }
}

void OMDemiurgeRendererHandler::afterFrame()
{
}
} // namespace openminecraft::renderer::common::demiurge
