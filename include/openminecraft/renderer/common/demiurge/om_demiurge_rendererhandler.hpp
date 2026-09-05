#ifndef OM_DEMIURGE_RENDERERHANDLER_HPP
#define OM_DEMIURGE_RENDERERHANDLER_HPP

#include "openminecraft/geom/om_fontset.hpp"
#include "openminecraft/log/om_log_common.hpp"
#include "openminecraft/renderer/common/demiurge/element/om_demiurge_element_image_channel.hpp"
#include "openminecraft/renderer/common/demiurge/element/om_demiurge_element_rect_channel.hpp"
#include "openminecraft/renderer/common/demiurge/element/om_demiurge_element_roundedrect_channel.hpp"
#include "openminecraft/renderer/common/demiurge/element/om_demiurge_element_sector_channel.hpp"
#include "openminecraft/renderer/common/demiurge/element/om_demiurge_element_textsdf_channel.hpp"
#include "openminecraft/renderer/common/demiurge/om_demiurge_node.hpp"
#include "openminecraft/renderer/common/om_renderer_buffer.hpp"
#include "openminecraft/renderer/common/om_renderer_handler.hpp"
#include "openminecraft/renderer/common/om_renderer_pipeline.hpp"
#include "openminecraft/renderer/common/om_renderer_rendertarget.hpp"
#include "openminecraft/renderer/common/om_renderer_task.hpp"
#include "openminecraft/renderer/common/om_renderer_texture.hpp"
#include "openminecraft/renderer/common/wrap/om_renderer_temptarget.hpp"
#include "openminecraft/renderer/om_renderer_layer.hpp"
#include <memory>
#include <unordered_map>
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
    OMDemiurgeRendererHandler(OMRenderer *renderer, std::shared_ptr<OMDemiurgeNode>);
    ~OMDemiurgeRendererHandler() override;

    void submitTasks() override;
    void beforeFrame() override;
    void afterFrame() override;

    void recordTask(bool resize = false);

    std::shared_ptr<OMDemiurgeNode> node;

    wrap::OMRendererTempTarget *middleTarget;
    OMRendererBuffer *uniformBuffer;
    OMRenderer *renderer;

    OMRendererBuffer *testBuffer;
    OMRendererPipeline *testPipeline;

    element::OMDemiurgeRectChannel rect;
    element::OMDemiurgeRoundedRectChannel roundedRect;
    element::OMDemiurgeImageChannel image;
    element::OMDemiurgeSectorChannel sector;

    auto fetchFontChannel(geom::OMFontSet *) -> std::shared_ptr<element::OMDemiurgeTextSdfChannel>;

    bool fit = false;

  private:
    std::unordered_map<geom::OMFontSet *, std::shared_ptr<element::OMDemiurgeTextSdfChannel>> fonts = {};

    log::OMLogger logger;
};

} // namespace openminecraft::renderer::common::demiurge

#endif
