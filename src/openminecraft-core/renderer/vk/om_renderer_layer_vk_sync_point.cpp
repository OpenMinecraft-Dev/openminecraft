#include "openminecraft/renderer/vk/om_renderer_layer_vk_sync_point.hpp"
#include "openminecraft/renderer/vk/om_renderer_layer_vk.hpp"
#include "openminecraft/renderer/om_renderer_exception.hpp"
#include "openminecraft/i18n/om_i18n_res.hpp"
#include <limits>

using namespace vk;
using namespace openminecraft::i18n::res;

namespace openminecraft::renderer::vk
{
OMRendererGpuEventVk::OMRendererGpuEventVk(Device device, AllocationCallbacks allocator)
    : device(device), allocator(allocator)
{
    handle = device.createSemaphore(SemaphoreCreateInfo(), allocator);
}

OMRendererGpuEventVk::~OMRendererGpuEventVk()
{
    if (handle)
    {
        device.destroySemaphore(handle, allocator);
    }
}

OMRendererGpuCpuBridgeVk::OMRendererGpuCpuBridgeVk(Device device, AllocationCallbacks allocator, bool signaled)
    : device(device), allocator(allocator)
{
    handle =
        device.createFence(FenceCreateInfo(signaled ? FenceCreateFlagBits::eSignaled : FenceCreateFlags{}), allocator);
}

OMRendererGpuCpuBridgeVk::~OMRendererGpuCpuBridgeVk()
{
    if (handle)
    {
        device.destroyFence(handle, allocator);
    }
}

void OMRendererGpuCpuBridgeVk::waitCpu()
{
    try
    {
        auto result = device.waitForFences(1, &handle, true, std::numeric_limits<uint64_t>::max());
    }
    catch (SystemError &e)
    {
        throw OMRendererException(VkErrorTranslate(e, "openminecraft.renderer.vk.err.waitfence"));
    }
}

auto OMRendererGpuCpuBridgeVk::isSignaled() -> bool
{
    return device.getFenceStatus(handle) == Result::eSuccess;
}

void OMRendererGpuCpuBridgeVk::reset()
{
    try
    {
        auto result = device.resetFences(1, &handle);
    }
    catch (SystemError &e)
    {
        throw OMRendererException(VkErrorTranslate(e, "openminecraft.renderer.vk.err.resetfence"));
    }
}
} // namespace openminecraft::renderer::vk
