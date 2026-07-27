#ifndef OM_DEMIURGE_RENDERERHANDLER_HPP
#define OM_DEMIURGE_RENDERERHANDLER_HPP

#include "openminecraft/log/om_log_common.hpp"
#include "openminecraft/renderer/common/demiurge/element/om_demiurge_element_rect_channel.hpp"
#include "openminecraft/renderer/common/demiurge/element/om_demiurge_element_roundedrect_channel.hpp"
#include "openminecraft/renderer/common/demiurge/om_demiurge_node.hpp"
#include "openminecraft/renderer/common/om_renderer_buffer.hpp"
#include "openminecraft/renderer/common/om_renderer_handler.hpp"
#include "openminecraft/renderer/common/om_renderer_pipeline.hpp"
#include "openminecraft/renderer/common/om_renderer_rendertarget.hpp"
#include "openminecraft/renderer/common/om_renderer_task.hpp"
#include "openminecraft/renderer/common/om_renderer_texture.hpp"
#include "openminecraft/renderer/om_renderer_layer.hpp"
namespace openminecraft::renderer::common::demiurge
{
constexpr float bottomDepth = 0.99f;
constexpr float topDepth = 0.01f;
constexpr float layerHalfWidth = 0.005f;

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

    element::OMDemiurgeRectChannel rect;
    element::OMDemiurgeRoundedRectChannel roundedRect;

  private:
    log::OMLogger logger;
};

} // namespace openminecraft::renderer::common::demiurge

#endif
