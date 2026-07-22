#include "openminecraft/renderer/common/demiurge/element/om_demiurge_rect.hpp"
#include "glm/ext/vector_float4.hpp"
#include "openminecraft/renderer/common/demiurge/om_demiurge_rendererhandler.hpp"
#include "openminecraft/renderer/common/demiurge/om_demiurge_srgb.hpp"
#include "openminecraft/renderer/common/om_renderer_buffer.hpp"
#include "openminecraft/renderer/common/om_renderer_task.hpp"
#include <any>
#include <array>

namespace openminecraft::renderer::common::demiurge::element
{
OMDemiurgeRectElement::OMDemiurgeRectElement() = default;
OMDemiurgeRectElement::~OMDemiurgeRectElement()
{
    if (indexBuffer || vertexBuffer)
    {
        delete indexBuffer;
        delete vertexBuffer;
        delete instanceBuffer;
    }
}

static std::vector<glm::vec4> colors = {
    genLinear(0x2c2c34ff),
    genLinear(0x00d4ffff),
    genLinear(0x89c2ffff),
    genLinear(0xe6f7ffff),
};
static int l = 0;

void OMDemiurgeRectElement::render(OMRendererTask *task, OMDemiurgeRendererHandler *handler, float depth)
{
    auto c = genLinear(std::any_cast<int>(styles["color"]));
    auto bound = boundary();
    std::array<float, 4 * 3> vtx = {
        bound.x,
        bound.y,
        depth,
        bound.x + bound.width,
        bound.y,
        depth,
        bound.x + bound.width,
        bound.y + bound.height,
        depth,
        bound.x,
        bound.y + bound.height,
        depth,
    };
    std::array<uint32_t, 6> idx = {0, 1, 2, 2, 3, 0};
    if (!indexBuffer || !vertexBuffer)
    {
        indexBuffer = handler->renderer->allocateBuffer(VertexIndex, 6 * sizeof(uint32_t));
        vertexBuffer = handler->renderer->allocateBuffer(VertexData, 3 * 4 * sizeof(float));
        instanceBuffer = handler->renderer->allocateBuffer(InstanceData, sizeof(glm::vec4));
    }

    indexBuffer->updateData(&idx);
    vertexBuffer->updateData(&vtx);
    instanceBuffer->updateData(&c);

    task->pipeline(handler->uiPipeline)
        ->vertexBuffer({vertexBuffer, instanceBuffer})
        ->indexBuffer(indexBuffer)
        ->drawInstanceN(6, 1);

    for (auto c : children)
    {
        c->render(task, handler, depth - 0.001f);
    }
}
} // namespace openminecraft::renderer::common::demiurge::element
