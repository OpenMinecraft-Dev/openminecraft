#ifndef COMPOSERENDERER_HPP
#define COMPOSERENDERER_HPP

#include "openminecraft/renderer/common/om_renderer_handler.hpp"
#include "openminecraft/renderer/common/om_renderer_texture.hpp"
#include "openminecraft/renderer/common/wrap/om_renderer_blur.hpp"
#include "openminecraft/renderer/om_renderer_layer.hpp"
namespace openminecraftshell::renderer
{
class OMComposeRenderer : public openminecraft::renderer::common::OMRendererHandler
{
  public:
    OMComposeRenderer(openminecraft::renderer::OMRenderer *renderer,
                      std::function<openminecraft::renderer::common::OMRendererTexture *()>,
                      std::function<openminecraft::renderer::common::OMRendererTexture *()>);
    virtual ~OMComposeRenderer() override;

    void submitTasks() override;
    void beforeFrame() override;
    void afterFrame() override;

  private:
    std::shared_ptr<openminecraft::renderer::common::wrap::OMRendererBlurHandler> blurHandler;
    openminecraft::renderer::common::OMRendererPipeline *mainPipeline;
    openminecraft::renderer::OMRenderer *renderer;

    std::function<openminecraft::renderer::common::OMRendererTexture *()> overlay, scene;
};
} // namespace openminecraftshell::renderer

#endif