#ifndef OM_RENDERER_LAYER_VK_HPP
#define OM_RENDERER_LAYER_VK_HPP

#include "openminecraft/log/om_log_common.hpp"
#include "openminecraft/renderer/om_renderer_layer.hpp"
#include <memory>
namespace openminecraft::renderer::vk
{
class OMRendererVk : public OMRenderer
{
  public:
    OMRendererVk(AppInfo info);
    ~OMRendererVk();

    virtual std::string driver() override;

  private:
    std::shared_ptr<log::OMLogger> logger;
};
}; // namespace openminecraft::renderer::vk

#endif