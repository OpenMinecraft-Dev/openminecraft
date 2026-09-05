#include "openminecraft/renderer/common/demiurge/om_demiurge_rendererhandler.hpp"
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

    auto comp = openminecraft::geom::svg::compile(
        openminecraft::geom::svg::mergeTo(openminecraft::geom::svg::parseSvgPath(
            "M3.25 4.8c.515 0 .773 0 .955.129q.097.069.166.166c.13.182.129.44.129.955v.7c0 .515 0 .773-.129.955a.7.7 0 "
            "0 1-.166.166c-.182.13-.44.129-.955.129s-.773 0-.955-.129a.7.7 0 0 1-.166-.166C1.999 7.523 2 7.265 2 "
            "6.75v-.7c0-.515 0-.773.129-.955a.7.7 0 0 1 .166-.166c.182-.13.44-.13.955-.13M7.1 6.2c.375 0 .563 0 "
            ".694.096a.5.5 0 0 1 .11.11C8 6.538 8 6.725 8 7.1s0 .563-.096.694a.5.5 0 0 1-.11.11C7.663 8 7.474 8 7.1 "
            "8h-.8c-.375 0-.562 0-.694-.096a.5.5 0 0 1-.11-.11C5.4 7.663 5.4 7.474 5.4 7.1c0-.375 0-.562.095-.694a.5.5 "
            "0 0 1 .111-.11c.132-.096.319-.096.694-.096zM6.75 2c.515 0 .773 0 "
            ".955.129q.097.069.166.166c.13.182.129.44.129.955v.7c0 .515 0 .773-.129.955a.7.7 0 0 "
            "1-.166.166c-.182.13-.44.13-.955.13s-.773 0-.955-.13a.7.7 0 0 "
            "1-.166-.166c-.13-.182-.129-.44-.129-.955v-.7c0-.515 0-.773.129-.955a.7.7 0 0 1 "
            ".166-.166c.182-.13.44-.129.955-.129M3.6 2c.375 0 .563 0 .694.096a.5.5 0 0 1 "
            ".11.11c.096.131.096.32.096.694 0 .375 0 .562-.096.694a.5.5 0 0 "
            "1-.11.11c-.131.096-.32.096-.694.096h-.7c-.375 0-.563 0-.694-.096a.5.5 0 0 1-.11-.11C2 3.462 2 3.275 2 "
            "2.9s0-.563.096-.694a.5.5 0 0 1 .11-.11C2.337 2 2.525 2 2.899 2z")),
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
            ->depth(false, false)
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
    for (auto &p : fonts)
    {
        p.second->update();
    }
}

void OMDemiurgeRendererHandler::afterFrame()
{
}
} // namespace openminecraft::renderer::common::demiurge
