#include "openminecraft/renderer/common/demiurge/element/om_demiurge_element_rect_channel.hpp"
#include "openminecraft/renderer/common/om_renderer_pipeline.hpp"
#include "openminecraft/vfs/om_vfs_base.hpp"
#include "openminecraft/io/om_io_utils.hpp"

namespace openminecraft::renderer::common::demiurge::element
{
void OMDemiurgeRectChannel::init(OMRendererBuffer *uniformBuffer, OMRendererRenderTarget *target)
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
    instanceBuffer = renderer->allocateBuffer(InstanceData, 8);

    pipeline = renderer->createPipeline()
                   ->input(UniformBuffer)
                   ->output(target)
                   ->shader(frgShader)
                   ->shader(vtxShader)
                   ->format(format)
                   ->blendFunc({Alpha, OneMinusAlpha, Alpha, OneMinusAlpha})
                   ->blend(true)
                   ->depth(false, false)
                   ->buildN();
    pipeline->bindInput(0, uniformBuffer);
}

void OMDemiurgeRectChannel::destroy()
{
    delete instanceBuffer;
    delete quadBuffer;
    delete quadIndex;
    delete pipeline;
}

void OMDemiurgeRectChannel::submitTask(OMRendererTask *task, float upper, float lower)
{
    if (instanceBuffer->length < bufferSize())
    {
        delete instanceBuffer;
        instanceBuffer = renderer->allocateBuffer(InstanceData, bufferSize());
    }
    task->pipeline(pipeline)->vertexBuffer({quadBuffer, instanceBuffer})->indexBuffer(quadIndex);

    int i = 0;
    for (auto &rect : objects)
    {
        if (rect.depth > lower && rect.depth < upper)
        {
            task->drawInstance(6, 1, i);
        }
        ++i;
    }
}

void OMDemiurgeRectChannel::update()
{
    if (instanceBuffer->length < bufferSize() || lastCount != objects.size())
    {
        lastCount = objects.size();
        // INFO: (fake) resize due to the instance buffer recreation
        renderer->requestResize();
        return;
    }

    dirtyIter([&](int begin, int length) -> void {
        instanceBuffer->updateDataPart(&objects[begin], begin * sizeof(element::OMDemiurgeElementRect),
                                       length * sizeof(element::OMDemiurgeElementRect));
    });
}
} // namespace openminecraft::renderer::common::demiurge::element
