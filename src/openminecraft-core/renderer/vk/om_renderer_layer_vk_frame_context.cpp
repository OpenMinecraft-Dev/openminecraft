#include "openminecraft/renderer/vk/om_renderer_layer_vk_frame_context.hpp"
#include "openminecraft/renderer/vk/om_renderer_layer_vk.hpp"
#include "openminecraft/renderer/vk/om_renderer_layer_vk_sync_point.hpp"
#include "openminecraft/renderer/om_renderer_exception.hpp"
#include "openminecraft/i18n/om_i18n_res.hpp"
#include <limits>

using namespace vk;
using namespace openminecraft::i18n::res;

namespace openminecraft::renderer::vk
{
OMRendererFrameContextVk::OMRendererFrameContextVk(OMRendererVk *renderer, uint32_t index)
    : renderer(renderer), frameIndex(index)
{
    pipelineSemaphores.push_back(reinterpret_cast<OMRendererGpuEventVk *>(renderer->createGpuEvent()));
    syncFence = renderer->createGpuCpuBridge(true);
}

OMRendererFrameContextVk::~OMRendererFrameContextVk()
{
    for (auto *sem : pipelineSemaphores)
    {
        delete sem;
    }
    pipelineSemaphores.clear();
    delete syncFence;
    syncFence = nullptr;
}

auto OMRendererFrameContextVk::imageAvailable() -> common::OMRendererGpuEvent *
{
    return pipelineSemaphores.front();
}

void OMRendererFrameContextVk::begin()
{
    beginFrame();
}

void OMRendererFrameContextVk::end()
{
}

auto OMRendererFrameContextVk::beginFrame() -> bool
{
    try
    {
        syncFence->waitCpu();

        auto event = reinterpret_cast<OMRendererGpuEventVk *>(imageAvailable());
        auto [result, image] = renderer->logicalDevice.acquireNextImageKHR(
            renderer->swapchainManager->swapchain, std::numeric_limits<uint64_t>::max(), event->semaphore(), {});

        if (result == Result::eErrorOutOfDateKHR || result == Result::eErrorSurfaceLostKHR)
        {
            return false;
        }
        if (result != Result::eSuccess && result != Result::eSuboptimalKHR)
        {
            throw OMRendererException(
                VkErrorTranslate(SystemError(result), "openminecraft.renderer.vk.err.nextimage"));
        }

        currentImage = image;

        auto inflight = renderer->inflightImageFences.find(image);
        if (inflight != renderer->inflightImageFences.end())
        {
            inflight->second->waitCpu();
        }

        renderer->inflightImageFences[image] = syncFence;
        reinterpret_cast<OMRendererGpuCpuBridgeVk *>(syncFence)->reset();
        ++frameSerial;
        return true;
    }
    catch (SystemError &e)
    {
        throw OMRendererException(VkErrorTranslate(e, "openminecraft.renderer.vk.err.nextimage"));
    }
}

auto OMRendererFrameContextVk::isFrameComplete(uint64_t serial) -> bool
{
    if (serial == 0)
    {
        return true;
    }
    if (serial < frameSerial)
    {
        return true;
    }
    if (serial > frameSerial)
    {
        return false;
    }
    return syncFence != nullptr && syncFence->isSignaled();
}

void OMRendererFrameContextVk::submit(const std::vector<CommandBuffer> &commands, uint32_t layer, bool lastLayer)
{
    if (layer >= pipelineSemaphores.size())
    {
        return;
    }

    auto *waitSync = pipelineSemaphores[layer];
    auto *dstSync = lastLayer ? renderer->imageRenderFinished[currentImage] : pipelineSemaphores[layer + 1];
    const PipelineStageFlags stages = PipelineStageFlagBits::eColorAttachmentOutput;
    Semaphore waitSemaphore = reinterpret_cast<OMRendererGpuEventVk *>(waitSync)->semaphore();
    Semaphore signalSemaphore = reinterpret_cast<OMRendererGpuEventVk *>(dstSync)->semaphore();

    SubmitInfo info(1, &waitSemaphore, &stages, static_cast<uint32_t>(commands.size()), commands.data(), 1,
                    &signalSemaphore);
    renderer->queues.first.submit(
        info, lastLayer ? reinterpret_cast<OMRendererGpuCpuBridgeVk *>(syncFence)->fence() : nullptr);
}

auto OMRendererFrameContextVk::present() -> bool
{
    auto semaphore = reinterpret_cast<OMRendererGpuEventVk *>(renderer->imageRenderFinished[currentImage])->semaphore();
    PresentInfoKHR info(1, &semaphore, 1, &renderer->swapchainManager->swapchain, &currentImage);
    auto result = renderer->queues.second.presentKHR(info);

    if (result == Result::eSuboptimalKHR || result == Result::eErrorOutOfDateKHR ||
        result == Result::eErrorSurfaceLostKHR)
    {
        return false;
    }
    if (result != Result::eSuccess)
    {
        throw OMRendererException(VkErrorTranslate(SystemError(result), "openminecraft.renderer.vk.err.queuepresent"));
    }

    return true;
}

void OMRendererFrameContextVk::ensurePipelineSemaphores(size_t count)
{
    while (pipelineSemaphores.size() < count)
    {
        pipelineSemaphores.push_back(renderer->createGpuEvent());
    }
    while (pipelineSemaphores.size() > count)
    {
        delete pipelineSemaphores.back();
        pipelineSemaphores.pop_back();
    }
}
} // namespace openminecraft::renderer::vk
