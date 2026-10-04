#ifndef OM_SURFACERENDERER_HPP
#define OM_SURFACERENDERER_HPP

#include "openminecraft/renderer/common/animation/om_animation_value.hpp"
#include "openminecraft/renderer/common/om_renderer_handler.hpp"
#include "openminecraft/renderer/common/demiurge/om_demiurge_node.hpp"
#include "openminecraft/renderer/common/om_renderer_handler.hpp"
#include "openminecraft/renderer/common/demiurge/om_demiurge_rendererhandler.hpp"
#include <functional>
namespace openminecraftshell::renderer
{
class OMSurfaceRenderer : public openminecraft::renderer::common::OMRendererHandler
{
  public:
    OMSurfaceRenderer(openminecraft::renderer::OMRenderer *renderer,
                      openminecraft::renderer::common::event::OMEventBusWrap &);
    virtual ~OMSurfaceRenderer() override;

    void submitTasks() override;
    void beforeFrame() override;
    void afterFrame() override;

    openminecraft::renderer::OMRenderer *renderer;
    std::shared_ptr<openminecraft::renderer::common::demiurge::OMDemiurgeNode> node;
    std::shared_ptr<openminecraft::renderer::common::demiurge::OMDemiurgeRendererHandler> internal;
    openminecraft::renderer::common::animation::OMAnimationValue<float> offset;

    std::shared_ptr<openminecraft::geom::OMFontSet> fontset;

    openminecraft::renderer::common::event::OMEventBusWrap &bus;
    void openScreen();
};
} // namespace openminecraftshell::renderer

#endif