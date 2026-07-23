#ifndef OM_DEMIURGE_RENDERERHANDLER_HPP
#define OM_DEMIURGE_RENDERERHANDLER_HPP

#include "openminecraft/renderer/common/basics/om_vertex_format.hpp"
#include "openminecraft/renderer/common/demiurge/element/om_demiurge_element_rect.hpp"
#include "openminecraft/renderer/common/demiurge/om_demiurge_node.hpp"
#include "openminecraft/renderer/common/om_renderer_buffer.hpp"
#include "openminecraft/renderer/common/om_renderer_handler.hpp"
#include "openminecraft/renderer/common/om_renderer_pipeline.hpp"
#include "openminecraft/renderer/common/om_renderer_shader.hpp"
#include "openminecraft/renderer/om_renderer_layer.hpp"
#include <vector>
namespace openminecraft::renderer::common::demiurge
{
struct OMDemiurgeIndirect
{
    uint32_t indexCount;
    uint32_t instanceCount;
    uint32_t firstIndex;
    int32_t vertexOffset;
    uint32_t firstInstance;
};
class OMDemiurgeRendererHandler : public OMRendererHandler
{
  public:
    OMDemiurgeRendererHandler(OMRenderer *renderer);
    ~OMDemiurgeRendererHandler() override;

    void submitTasks() override;
    void beforeFrame() override;
    void afterFrame() override;

    std::shared_ptr<OMDemiurgeNode> node;

    OMRendererBuffer *uniformBuffer;
    OMRenderer *renderer;

    struct
    {
        OMRendererBuffer *quadBuffer;
        OMRendererBuffer *quadIndex;
        OMRendererBuffer *indirectBuffer;
        OMRendererBuffer *instanceBuffer = nullptr;

        std::vector<element::OMDemiurgeElementRect> rects = {};

        OMRendererPipeline *pipeline;
        std::shared_ptr<OMShader> vtxShader, frgShader;
        basics::OMVertexFormat format;

        auto request() -> int
        {
            rects.emplace_back(element::OMDemiurgeElementRect{});
            return rects.size() - 1;
        }

        auto temporary(int i) -> element::OMDemiurgeElementRect *
        {
            return &rects[i];
        }
    } rect;
};
} // namespace openminecraft::renderer::common::demiurge

#endif
