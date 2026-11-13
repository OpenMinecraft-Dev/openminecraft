#ifndef OM_DEMIURGE_RENDERERHANDLER_HPP
#define OM_DEMIURGE_RENDERERHANDLER_HPP

#include "openminecraft/geom/om_fontset.hpp"
#include "openminecraft/log/om_log_common.hpp"
#include "openminecraft/renderer/common/demiurge/element/om_demiurge_element_image_channel.hpp"
#include "openminecraft/renderer/common/demiurge/element/om_demiurge_element_rect_channel.hpp"
#include "openminecraft/renderer/common/demiurge/element/om_demiurge_element_shaderscene_channel.hpp"
#include "openminecraft/renderer/common/demiurge/element/om_demiurge_element_roundedrect_channel.hpp"
#include "openminecraft/renderer/common/demiurge/element/om_demiurge_element_sector_channel.hpp"
#include "openminecraft/renderer/common/demiurge/element/om_demiurge_element_svg_channel.hpp"
#include "openminecraft/renderer/common/demiurge/element/om_demiurge_element_textsdf_channel.hpp"
#include "openminecraft/renderer/common/demiurge/om_demiurge_node.hpp"
#include "openminecraft/renderer/common/om_renderer_buffer.hpp"
#include "openminecraft/renderer/common/om_renderer_handler.hpp"
#include "openminecraft/renderer/common/om_renderer_pipeline.hpp"
#include "openminecraft/renderer/common/om_renderer_rendertarget.hpp"
#include "openminecraft/renderer/common/om_renderer_task.hpp"
#include "openminecraft/renderer/common/om_renderer_texture.hpp"
#include "openminecraft/renderer/common/wrap/om_renderer_object_ref.hpp"
#include "openminecraft/renderer/common/wrap/om_renderer_temptarget.hpp"
#include "openminecraft/renderer/om_renderer_layer.hpp"
#include <memory>
#include <unordered_map>
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
    OMDemiurgeRendererHandler(OMRenderer *renderer, std::shared_ptr<OMDemiurgeNode>);
    ~OMDemiurgeRendererHandler() override;

    void submitTasks() override;
    void beforeFrame() override;
    void afterFrame() override;

    void recordTask(bool resize = false);

    struct LayerBounds
    {
        uint32_t maxLayer = 0;
        uint32_t maxOverlayLayer = 0;
        bool hasOverlay = false;
    };
    auto currentLayerBounds() -> LayerBounds;

    std::shared_ptr<OMDemiurgeNode> node;

    wrap::OMRendererTempTarget *middleTarget;
    wrap::OMBufferRef uniformBuffer;
    OMRenderer *renderer;

    element::OMDemiurgeRectChannel rect;
    element::OMDemiurgeRoundedRectChannel roundedRect;
    element::OMDemiurgeImageChannel image;
    element::OMDemiurgeSectorChannel sector;
    element::OMDemiurgeSvgChannel svg;
    element::OMDemiurgeShaderSceneChannel shaderScene;

    auto fetchFontChannel(geom::OMFontSet *) -> std::shared_ptr<element::OMDemiurgeTextSdfChannel>;

    // Converts a logical (layout) clip rect into target pixels and stores it on the
    // channel. Keeping the conversion here lets the UI be rendered at a different
    // scale than the layout without touching the element channels.
    template <typename Channel>
    auto applyClip(Channel &channel, int index, const std::optional<OMDemiurgeRect> &bound) -> void
    {
        if (!bound.has_value())
        {
            if (channel.setClip(index, std::nullopt))
            {
                clipDirty = true;
            }

            return;
        }

        const auto extent = renderer->getExtent();
        const auto logical = renderer->getLogicalExtent() / renderer->getUiScale();
        const float scaleX = logical.x > 0 ? static_cast<float>(extent.x) / static_cast<float>(logical.x) : 1.0f;
        const float scaleY = logical.y > 0 ? static_cast<float>(extent.y) / static_cast<float>(logical.y) : 1.0f;

        if (channel.setClip(index, OMDemiurgeRect{bound->x * scaleX, bound->y * scaleY, bound->width * scaleX,
                                                  bound->height * scaleY}))
        {
            clipDirty = true;
        }
    }

    bool fit = false;
    bool clipDirty = false;
    LayerBounds recordedLayerBounds;
    std::string name = "default";

  private:
    std::unordered_map<geom::OMFontSet *, std::shared_ptr<element::OMDemiurgeTextSdfChannel>> fonts = {};

    log::OMLogger logger;
};

} // namespace openminecraft::renderer::common::demiurge

#endif
