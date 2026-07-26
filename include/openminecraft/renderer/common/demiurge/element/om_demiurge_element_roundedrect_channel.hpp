#ifndef OM_DEMIURGE_ELEMENT_ROUNDEDRECT_CHANNEL_HPP
#define OM_DEMIURGE_ELEMENT_ROUNDEDRECT_CHANNEL_HPP

#include "openminecraft/renderer/common/demiurge/element/om_demiurge_element_roundedrect.hpp"
#include "openminecraft/renderer/common/demiurge/element/om_demiurge_element_channel.hpp"
#include "openminecraft/renderer/om_renderer_layer.hpp"
namespace openminecraft::renderer::common::demiurge::element
{
class OMDemiurgeRoundedRectChannel : public OMDemiurgeChannel<OMDemiurgeElementRoundedRect>
{
  public:
    OMDemiurgeRoundedRectChannel(OMRenderer *renderer) : renderer(renderer)
    {
    }
    ~OMDemiurgeRoundedRectChannel() = default;

    void init(OMRendererBuffer *uniform, OMRendererRenderTarget *target) override;
    void submitTask(OMRendererTask *task) override;
    void update() override;
    void destroy() override;

  private:
    OMRenderer *renderer;
    OMRendererBuffer *quadBuffer;
    OMRendererBuffer *quadIndex;
    OMRendererBuffer *indirectBuffer;
    OMRendererBuffer *instanceBuffer = nullptr;

    OMRendererPipeline *pipeline;
    std::shared_ptr<OMShader> vtxShader, frgShader;
    basics::OMVertexFormat format;
};
} // namespace openminecraft::renderer::common::demiurge::element

#endif
