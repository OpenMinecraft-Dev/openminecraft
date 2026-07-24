#include "openminecraft/renderer/common/demiurge/om_demiurge_rendererhandler.hpp"
#include "glm/ext/vector_float2.hpp"
#include "openminecraft/renderer/common/basics/om_vertex_format.hpp"
#include "openminecraft/renderer/common/demiurge/element/om_demiurge_element_rect.hpp"
#include "openminecraft/renderer/common/demiurge/node/om_demiurge_rect.hpp"
#include "openminecraft/renderer/common/demiurge/om_demiurge_geometry.hpp"
#include "openminecraft/renderer/common/om_renderer_buffer.hpp"
#include "openminecraft/renderer/common/om_renderer_pipeline.hpp"
#include "openminecraft/renderer/om_renderer_layer.hpp"
#include <iostream>
#include <memory>
#include "openminecraft/vfs/om_vfs_base.hpp"
#include "openminecraft/io/om_io_utils.hpp"

namespace openminecraft::renderer::common::demiurge
{
struct SimpleUniform
{
    float width;
    float height;
};
OMDemiurgeRendererHandler::OMDemiurgeRendererHandler(OMRenderer *renderer)
    : renderer(renderer), OMRendererHandler(renderer)
{
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

#define shaderDef(name, filename, type)                                                                                \
    {                                                                                                                  \
        auto target = vfs::fsfetch(fmt::format("/bootassets/openminecraft-renderer/shaders/{}", filename));            \
        name = std::make_shared<OMShader>(GLSLSource, io::readOnce(target.get()), filename, "main", type);             \
    }
    shaderDef(rect.vtxShader, "demiurge/rect.vert.glsl", Vertex);
    shaderDef(rect.frgShader, "demiurge/rect.frag.glsl", Fragment);

    rect.format.appendPart("position", basics::Vec2f);
    rect.format.nextGroup();
    rect.format.setInstance();
    rect.format.appendPart("rect_pos", basics::Vec4f);
    rect.format.appendPart("rect_color", basics::Vec4f);
    rect.format.appendPart("rect_depth", basics::Float);
    rect.format.nextGroup();
    rect.format.decideStruct();
    rect.format.debugState();

    rect.quadBuffer = renderer->allocateBuffer(VertexData, 4 * sizeof(glm::vec2));
    rect.quadBuffer->updateData(
        std::array<glm::vec2, 4>{{{0.0f, 0.0f}, {0.0f, 1.0f}, {1.0f, 1.0f}, {1.0f, 0.0f}}}.data());
    rect.quadIndex = renderer->allocateBuffer(VertexIndex, 6 * sizeof(uint32_t));
    rect.quadIndex->updateData(std::array<uint32_t, 6>{{0, 1, 2, 2, 3, 0}}.data());
    rect.indirectBuffer = renderer->allocateBuffer(Indirect, sizeof(OMDemiurgeIndirect));
    rect.instanceBuffer = renderer->allocateBuffer(InstanceData, 8);

    uniformBuffer = renderer->allocateBuffer(Uniform, sizeof(SimpleUniform));

    rect.pipeline = renderer->createPipeline()
                        ->input(UniformBuffer)
                        ->output(renderer->getDefaultRenderTarget())
                        ->shader(rect.frgShader)
                        ->shader(rect.vtxShader)
                        ->format(rect.format)
                        ->buildN();
    rect.pipeline->bindInput(0, uniformBuffer);
}
OMDemiurgeRendererHandler::~OMDemiurgeRendererHandler()
{
    delete rect.indirectBuffer;
    delete rect.instanceBuffer;
    delete rect.quadBuffer;
    delete rect.quadIndex;
    delete uniformBuffer;
    delete rect.pipeline;
}

void OMDemiurgeRendererHandler::submitTasks()
{
    auto ext = renderer->getExtent();
    SimpleUniform u{ext.x, ext.y};
    uniformBuffer->updateData(&u);

    auto tgt = sizeof(element::OMDemiurgeElementRect) * rect.rects.size();

    if (!rect.instanceBuffer || rect.instanceBuffer->length < tgt)
    {
        delete rect.instanceBuffer;
        rect.instanceBuffer = renderer->allocateBuffer(InstanceData, tgt);
    }

    auto task = renderer->createTask()
                    ->dependOn(renderer->fetchTask("main"))
                    ->target(renderer->getDefaultRenderTarget())
                    ->clearN()
                    ->pipeline(rect.pipeline)
                    ->vertexBuffer({rect.quadBuffer, rect.instanceBuffer})
                    ->indexBuffer(rect.quadIndex)
                    ->indirectBuffer(rect.indirectBuffer)
                    ->drawIndirectN(0, 1)
                    ->finishN();
    renderer->registerTask("demiurgeui_core", task);

    srand(time(nullptr));
}
void OMDemiurgeRendererHandler::beforeFrame()
{
    auto tgt = sizeof(element::OMDemiurgeElementRect) * rect.rects.size();

    if (!rect.instanceBuffer || rect.instanceBuffer->length < tgt)
    {
        // INFO: (fake) resize due to the instance buffer recreation
        renderer->requestResize();
        return;
    }
    auto ext = renderer->getExtent();

    node->layout(ext.x, ext.y);
    node->submit(this, 0.9f);

    if (std::find(rect.dirty.begin(), rect.dirty.end(), true) != rect.dirty.end())
    {
        rect.instanceBuffer->updateData(rect.rects.data());
    }
    rect.solve();

    OMDemiurgeIndirect i{6, static_cast<uint32_t>(rect.rects.size()), 0, 0, 0};
    rect.indirectBuffer->updateData(&i);
}
void OMDemiurgeRendererHandler::afterFrame()
{
}
} // namespace openminecraft::renderer::common::demiurge
