#ifndef OM_RENDERER_FRAME_CONTEXT_HPP
#define OM_RENDERER_FRAME_CONTEXT_HPP

#include "openminecraft/renderer/om_renderer_object.hpp"
#include <cstdint>

namespace openminecraft::renderer::common
{
// Owns the per-frame synchronization and resources used by one in-flight
// frame. Backends may expose additional methods, but all frame-slot state
// belongs to this object rather than to the global renderer.
class OMRendererFrameContext : public OMRendererObject
{
  public:
    ~OMRendererFrameContext() override = default;

    virtual auto index() const -> uint32_t = 0;
    virtual void begin() = 0;
    virtual void end() = 0;

    auto objType() -> OMRendererObjectType override
    {
        return FrameContext;
    }
};
} // namespace openminecraft::renderer::common

#endif
