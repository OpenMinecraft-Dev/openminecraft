#include "openminecraft/renderer/vk/om_renderer_layer_vk_validation.hpp"
#include "openminecraft/renderer/vk/om_renderer_layer_vk.hpp"
#include "openminecraft/i18n/om_i18n_res.hpp"
#include "openminecraft/log/om_log_common.hpp"
#include "openminecraft/renderer/vk/om_renderer_layer_vk.hpp"
#include "vulkan/vulkan_core.h"
#include "vulkan/vulkan_enums.hpp"
#include <cstdio>
#include <vector>
#ifdef OM_VULKAN_DYNAMIC
#define VULKAN_HPP_DISPATCH_LOADER_DYNAMIC 1
#endif
#include "vulkan/vulkan.hpp"
#include <memory>

using namespace vk;
using openminecraft::i18n::res::translate;
namespace openminecraft::renderer::vk::validation
{
log::OMLogger internal("Vulkan Validation");

static auto notify(DebugUtilsMessageSeverityFlagBitsEXT s, DebugUtilsMessageTypeFlagsEXT t,
                   DebugUtilsMessengerCallbackDataEXT data, void *user) -> int
{
    if (!data.pMessage)
    {
        return VK_SUCCESS;
    }

    internal.debug("{}", (void *)data.pMessageIdName);
    switch (s)
    {
    default:
    case DebugUtilsMessageSeverityFlagBitsEXT::eVerbose:
        internal.debug("{}", data.pMessage);
        break;
    case DebugUtilsMessageSeverityFlagBitsEXT::eInfo:
        internal.info("{}", data.pMessage);
        break;
    case DebugUtilsMessageSeverityFlagBitsEXT::eWarning:
        internal.warn("{}", data.pMessage);
        break;
    case DebugUtilsMessageSeverityFlagBitsEXT::eError:
        internal.error("{}", data.pMessage);
        break;
    }

    return VK_SUCCESS;
}
static auto notifyNew(DebugReportFlagBitsEXT flags, DebugReportObjectTypeEXT objectType, uint64_t object,
                      size_t location, int32_t messageCode, const char *pLayerPrefix, const char *pMessage,
                      void *pUserData) -> VkBool32
{
    switch (flags)
    {
    case DebugReportFlagBitsEXT::eDebug:
        internal.debug("{}", pMessage);
        break;
    default:
    case DebugReportFlagBitsEXT::eInformation:
        internal.info("{}", pMessage);
        break;
    case DebugReportFlagBitsEXT::eWarning:
    case DebugReportFlagBitsEXT::ePerformanceWarning:
        internal.warn("{}", pMessage);
        break;
    case DebugReportFlagBitsEXT::eError:
        internal.error("{}", pMessage);
        internal.dumpStacktrace();
        break;
    }
    return false;
}
OMRendererVkValidation::OMRendererVkValidation(std::vector<LayerProperties> props)
{
    logger = std::make_shared<log::OMLogger>("OMRendererVkValidation", this);
    enabled = false;
    for (auto p : props)
    {
        if (std::string(p.layerName.data()) == "VK_LAYER_KHRONOS_validation")
        {
            goto baseinit;
        }
    }
    logger->info(translate("openminecraft.renderer.vk.validationdisable"));
    return;
baseinit:
    enabled = true;
    callbackInfo = {DebugReportFlagBitsEXT::eDebug | DebugReportFlagBitsEXT::eInformation |
                        DebugReportFlagBitsEXT::eWarning | DebugReportFlagBitsEXT::ePerformanceWarning |
                        DebugReportFlagBitsEXT::eError,
                    PFN_DebugReportCallbackEXT(notifyNew), nullptr, nullptr};
}
void OMRendererVkValidation::attachExts(std::vector<const char *> *data)
{
    if (enabled)
    {
        data->push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
        data->push_back(VK_EXT_DEBUG_REPORT_EXTENSION_NAME);
    }
}
void OMRendererVkValidation::attach(std::vector<const char *> *data)
{
    if (enabled)
    {
        data->push_back("VK_LAYER_KHRONOS_validation");
    }
}
void OMRendererVkValidation::ifEnable(std::function<void()> func)
{
    if (enabled)
    {
        func();
    }
}
OMRendererVkValidation::~OMRendererVkValidation() = default;
} // namespace openminecraft::renderer::vk::validation
