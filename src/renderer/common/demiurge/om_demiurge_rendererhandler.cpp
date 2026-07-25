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
    : renderer(renderer), OMRendererHandler(renderer), rect(renderer)
{
    std::cout << 4 << std::endl;
    node = std::make_shared<node::OMDemiurgeRectNode>()->style({
        {"color", 0x2c2c34ff},
        {"flexGap", 10_px},
        {"flexWrap", Wrap},
        {"flexDirection", Row},
    });
    std::cout << 4 << std::endl;
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
    std::cout << 4 << std::endl;
    uniformBuffer = renderer->allocateBuffer(Uniform, sizeof(SimpleUniform));
    std::cout << 4 << std::endl;
    rect.init(uniformBuffer);
    std::cout << 4 << std::endl;
}

void OMDemiurgeRectChannel::init(OMRendererBuffer *uniformBuffer)
{
#define shaderDef(name, filename, type)                                                                                \
    {                                                                                                                  \
        auto target = vfs::fsfetch(fmt::format("/bootassets/openminecraft-renderer/shaders/{}", filename));            \
        name = std::make_shared<OMShader>(GLSLSource, io::readOnce(target.get()), filename, "main", type);             \
    }
    shaderDef(vtxShader, "demiurge/rect.vert.glsl", Vertex);
    shaderDef(frgShader, "demiurge/rect.frag.glsl", Fragment);

    format.appendPart("position", basics::Vec2f);
    format.nextGroup();
    format.setInstance();
    format.appendPart("rect_pos", basics::Vec4f);
    format.appendPart("rect_color", basics::Vec4f);
    format.appendPart("rect_depth", basics::Float);
    format.nextGroup();
    format.decideStruct();
    format.debugState();

    quadBuffer = renderer->allocateBuffer(VertexData, 4 * sizeof(glm::vec2));
    quadBuffer->updateData(std::array<glm::vec2, 4>{{{0.0f, 0.0f}, {0.0f, 1.0f}, {1.0f, 1.0f}, {1.0f, 0.0f}}}.data());
    quadIndex = renderer->allocateBuffer(VertexIndex, 6 * sizeof(uint32_t));
    quadIndex->updateData(std::array<uint32_t, 6>{{0, 1, 2, 2, 3, 0}}.data());
    indirectBuffer = renderer->allocateBuffer(Indirect, sizeof(OMDemiurgeIndirect));
    instanceBuffer = renderer->allocateBuffer(InstanceData, 8);

    pipeline = renderer->createPipeline()
                   ->input(UniformBuffer)
                   ->output(renderer->getDefaultRenderTarget())
                   ->shader(frgShader)
                   ->shader(vtxShader)
                   ->format(format)
                   ->buildN();
    pipeline->bindInput(0, uniformBuffer);
}

OMDemiurgeRendererHandler::~OMDemiurgeRendererHandler()
{
    rect.destroy();
    delete uniformBuffer;
}

void OMDemiurgeRectChannel::destroy()
{
    delete indirectBuffer;
    delete instanceBuffer;
    delete quadBuffer;
    delete quadIndex;
    delete pipeline;
}

void OMDemiurgeRectChannel::submitTask(OMRendererTask *task)
{
    auto tgt = sizeof(element::OMDemiurgeElementRect) * rects.size();

    if (!instanceBuffer || instanceBuffer->length < tgt)
    {
        delete instanceBuffer;
        instanceBuffer = renderer->allocateBuffer(InstanceData, tgt);
    }
    task->pipeline(pipeline)
        ->vertexBuffer({quadBuffer, instanceBuffer})
        ->indexBuffer(quadIndex)
        ->indirectBuffer(indirectBuffer)
        ->drawIndirect(0, 1);
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
    rect.submitTask(task);
    task->finish();

    renderer->registerTask("demiurgeui_core", task);
}

void OMDemiurgeRectChannel::update()
{
    auto tgt = sizeof(element::OMDemiurgeElementRect) * rects.size();

    if (!instanceBuffer || instanceBuffer->length < tgt)
    {
        // INFO: (fake) resize due to the instance buffer recreation
        renderer->requestResize();
        return;
    }

    if (std::find(dirty.begin(), dirty.end(), true) != dirty.end())
    {
        bool in_dirty = false;
        int start = 0;
        for (int i = 0; i <= dirty.size(); ++i)
        {
            bool is_dirty = (i < dirty.size()) && dirty[i];
            if (!in_dirty && is_dirty)
            {
                start = i;
                in_dirty = true;
            }
            else if (in_dirty && !is_dirty)
            {
                instanceBuffer->updateDataPart(&rects[start], start * sizeof(element::OMDemiurgeElementRect),
                                               (i - start) * sizeof(element::OMDemiurgeElementRect));
                in_dirty = false;
            }
        }
        solve();
    }

    OMDemiurgeIndirect i{6, static_cast<uint32_t>(rects.size()), 0, 0, 0};
    indirectBuffer->updateData(&i);
}

void OMDemiurgeRendererHandler::beforeFrame()
{
    auto ext = renderer->getExtent();

    node->layout(ext.x, ext.y);
    node->submit(this, 0.9f);

    rect.update();
}

void OMDemiurgeRendererHandler::afterFrame()
{
}
} // namespace openminecraft::renderer::common::demiurge
