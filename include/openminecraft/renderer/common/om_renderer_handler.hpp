#ifndef OM_RENDERER_APP_HPP
#define OM_RENDERER_APP_HPP

#include "openminecraft/renderer/om_renderer_layer.hpp"
namespace openminecraft::renderer::common
{
class OMRendererHandler
{
  public:
    OMRendererHandler(OMRenderer *renderer)
    {
    }
    virtual ~OMRendererHandler()
    {
    }

    virtual void beforeFrame() = 0;
    virtual void afterFrame() = 0;
    virtual void onResize() = 0;
};
} // namespace openminecraft::renderer::common

#endif
