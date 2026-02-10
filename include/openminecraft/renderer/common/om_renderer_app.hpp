#ifndef OM_RENDERER_APP_HPP
#define OM_RENDERER_APP_HPP

namespace openminecraft::renderer::common
{
class OMRendererApp
{
  public:
    OMRendererApp() = default;
    virtual ~OMRendererApp()
    {
    }

    virtual void beforeFrame() = 0;
    virtual void afterFrame() = 0;
    virtual void onResize() = 0;
};
} // namespace openminecraft::renderer::common

#endif
