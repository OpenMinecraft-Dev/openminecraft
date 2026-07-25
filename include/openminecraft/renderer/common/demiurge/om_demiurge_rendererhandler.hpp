#ifndef OM_DEMIURGE_RENDERERHANDLER_HPP
#define OM_DEMIURGE_RENDERERHANDLER_HPP

#include "openminecraft/renderer/common/basics/om_vertex_format.hpp"
#include "openminecraft/renderer/common/demiurge/element/om_demiurge_element_rect.hpp"
#include "openminecraft/renderer/common/demiurge/om_demiurge_node.hpp"
#include "openminecraft/renderer/common/om_renderer_buffer.hpp"
#include "openminecraft/renderer/common/om_renderer_handler.hpp"
#include "openminecraft/renderer/common/om_renderer_pipeline.hpp"
#include "openminecraft/renderer/common/om_renderer_shader.hpp"
#include "openminecraft/renderer/common/om_renderer_task.hpp"
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

class OMDemiurgeRectChannel
{
  public:
    OMDemiurgeRectChannel(OMRenderer *renderer) : renderer(renderer)
    {
    }
    ~OMDemiurgeRectChannel() = default;

    void init(OMRendererBuffer *uniform);
    void submitTask(OMRendererTask *task);
    void update();
    void destroy();

    inline auto request() -> int
    {
        rects.emplace_back(element::OMDemiurgeElementRect{});
        dirty.resize(rects.size());
        return rects.size() - 1;
    }

    inline auto temporary(int i) -> element::OMDemiurgeElementRect *
    {
        dirty[i] = true;
        return &rects[i];
    }

    inline auto solve() -> void
    {
        dirty.assign(dirty.size(), false);
    }

  private:
    OMRenderer *renderer;
    OMRendererBuffer *quadBuffer;
    OMRendererBuffer *quadIndex;
    OMRendererBuffer *indirectBuffer;
    OMRendererBuffer *instanceBuffer = nullptr;

    std::vector<element::OMDemiurgeElementRect> rects = {};
    std::vector<bool> dirty = {};

    OMRendererPipeline *pipeline;
    std::shared_ptr<OMShader> vtxShader, frgShader;
    basics::OMVertexFormat format;
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

    OMDemiurgeRectChannel rect;
};

} // namespace openminecraft::renderer::common::demiurge

#endif
