#ifndef OM_RENDERER_APP_HPP
#define OM_RENDERER_APP_HPP

#include "openminecraft/renderer/om_renderer_layer.hpp"
#include "openminecraft/renderer/om_renderer_object.hpp"
namespace openminecraft::renderer::common
{
class OMRendererHandler : public OMRendererObject
{
  public:
    OMRendererHandler(OMRenderer *renderer)
    {
    }
    virtual ~OMRendererHandler() = default;

    virtual void beforeFrame() = 0;
    virtual void afterFrame() = 0;
    virtual void submitTasks() = 0;

    inline auto objType() -> OMRendererObjectType override
    {
        return Handler;
    }
};
} // namespace openminecraft::renderer::common

#endif
