#ifndef OM_RENDERER_LAYER_OPENGL_SYNC_POINT_HPP
#define OM_RENDERER_LAYER_OPENGL_SYNC_POINT_HPP

#include "openminecraft/renderer/common/om_renderer_sync_point.hpp"

namespace openminecraft::renderer::opengl
{
class OMRendererGpuEventOpenGL : public common::OMRendererGpuEvent
{
  public:
    OMRendererGpuEventOpenGL() = default;
    ~OMRendererGpuEventOpenGL() override = default;
};

class OMRendererGpuCpuBridgeOpenGL : public common::OMRendererGpuCpuBridge
{
  public:
    OMRendererGpuCpuBridgeOpenGL() = default;
    ~OMRendererGpuCpuBridgeOpenGL() override = default;

    void waitCpu() override
    {
    }

    auto isSignaled() -> bool
    {
        return true;
    }

    void reset()
    {
    }
};
} // namespace openminecraft::renderer::opengl

#endif
