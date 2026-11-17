#ifndef OM_RENDERER_SYNC_POINT_HPP
#define OM_RENDERER_SYNC_POINT_HPP

#include "openminecraft/renderer/om_renderer_object.hpp"
#include <stdexcept>
#include <string>

namespace openminecraft::renderer::common
{
// A synchronization primitive used by the RHI.
//
// GPU event:
//   GPU-GPU only. It may be waited by a GPU queue, but CPU must never wait on it.
//
// GPU-CPU bridge:
//   GPU-CPU synchronization. The CPU may wait on it (normally at frame boundaries).
class OMRendererSyncPoint : public OMRendererObject
{
  public:
    enum class Kind
    {
        GpuEvent,
        GpuCpuBridge
    };

    ~OMRendererSyncPoint() override = default;

    virtual auto kind() const -> Kind = 0;
    auto isGpuEvent() const -> bool
    {
        return kind() == Kind::GpuEvent;
    }
    auto isCpuWaitable() const -> bool
    {
        return kind() == Kind::GpuCpuBridge;
    }

    // CPU wait. GPU event implementations must reject this call.
    virtual void waitCpu() = 0;

    auto objType() -> OMRendererObjectType override
    {
        return SyncPoint;
    }
};

class OMRendererGpuEvent : public OMRendererSyncPoint
{
  public:
    auto kind() const -> Kind override
    {
        return Kind::GpuEvent;
    }

    void waitCpu() override
    {
        throw std::runtime_error("GPU event SyncPoint cannot be waited on by the CPU");
    }
};

class OMRendererGpuCpuBridge : public OMRendererSyncPoint
{
  public:
    auto kind() const -> Kind override
    {
        return Kind::GpuCpuBridge;
    }
};
} // namespace openminecraft::renderer::common

#endif
