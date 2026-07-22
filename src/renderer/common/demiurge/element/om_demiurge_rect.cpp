#include "openminecraft/renderer/common/demiurge/element/om_demiurge_rect.hpp"
#include "openminecraft/renderer/common/demiurge/om_demiurge_rendererhandler.hpp"
#include "openminecraft/renderer/common/om_renderer_buffer.hpp"
#include "openminecraft/renderer/common/om_renderer_task.hpp"
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
    }
}

void OMDemiurgeRectElement::render(OMRendererTask *task, OMDemiurgeRendererHandler *handler)
{
    auto bound = boundary();
    std::array<float, 4 * 7> vtx = {
        bound.x,
        bound.y,
        0,
        1,
        0,
        1,
        0,
        bound.x + bound.width,
        bound.y,
        0,
        1,
        0,
        1,
        0,
        bound.x + bound.width,
        bound.y + bound.height,
        0,
        1,
        0,
        1,
        0,
        bound.x,
        bound.y + bound.height,
        0,
        1,
        0,
        1,
        0,
    };
    std::array<uint32_t, 6> idx = {0, 1, 2, 2, 3, 0};
    if (!indexBuffer || !vertexBuffer)
    {
        indexBuffer = handler->renderer->allocateBuffer(VertexIndex, 6 * sizeof(uint32_t));
        vertexBuffer = handler->renderer->allocateBuffer(VertexData, 7 * 4 * sizeof(float));
    }

    indexBuffer->updateData(&idx);
    vertexBuffer->updateData(&vtx);

    task->pipeline(handler->uiPipeline)->vertexBuffer({vertexBuffer})->indexBuffer(indexBuffer)->drawN(6);
}
} // namespace openminecraft::renderer::common::demiurge::element
