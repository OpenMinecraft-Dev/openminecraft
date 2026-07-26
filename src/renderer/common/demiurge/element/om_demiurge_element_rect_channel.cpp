#include "openminecraft/renderer/common/demiurge/element/om_demiurge_element_rect_channel.hpp"
#include "openminecraft/vfs/om_vfs_base.hpp"
#include "openminecraft/io/om_io_utils.hpp"
#include "openminecraft/renderer/common/demiurge/om_demiurge_rendererhandler.hpp"

namespace openminecraft::renderer::common::demiurge::element
{
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
    if (instanceBuffer->length < bufferSize())
    {
        delete instanceBuffer;
        instanceBuffer = renderer->allocateBuffer(InstanceData, bufferSize());
    }
    task->pipeline(pipeline)
        ->vertexBuffer({quadBuffer, instanceBuffer})
        ->indexBuffer(quadIndex)
        ->indirectBuffer(indirectBuffer)
        ->drawIndirect(0, 1);
}

void OMDemiurgeRectChannel::update()
{
    if (instanceBuffer->length < bufferSize())
    {
        // INFO: (fake) resize due to the instance buffer recreation
        renderer->requestResize();
        return;
    }

    dirtyIter([&](int begin, int length) -> void {
        instanceBuffer->updateDataPart(&objects[begin], begin * sizeof(element::OMDemiurgeElementRect),
                                       length * sizeof(element::OMDemiurgeElementRect));
    });

    if (lastCount != objects.size())
    {
        OMDemiurgeIndirect i{6, static_cast<uint32_t>(objects.size()), 0, 0, 0};
        indirectBuffer->updateData(&i);
        lastCount = objects.size();
    }
}
} // namespace openminecraft::renderer::common::demiurge::element
