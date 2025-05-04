#include "openminecraft/renderer/vk/om_renderer_layer_vk_validation.hpp"
#include "openminecraft/i18n/om_i18n_res.hpp"
#include "openminecraft/log/om_log_common.hpp"
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
int notify(DebugUtilsMessageSeverityFlagBitsEXT s, DebugUtilsMessageTypeFlagsEXT t,
           DebugUtilsMessengerCallbackDataEXT data, void *user)
{
    internal.info("{}", data.pMessage);
    return VK_SUCCESS;
}
OMRendererVkValidation::OMRendererVkValidation(std::vector<LayerProperties> props)
{
    logger = std::make_shared<log::OMLogger>("OMRendererVkValidation", this);
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
    createInfo = DebugUtilsMessengerCreateInfoEXT(
        DebugUtilsMessengerCreateFlagsEXT(),
        DebugUtilsMessageSeverityFlagsEXT(
            -VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT |
            VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT),
        DebugUtilsMessageTypeFlagsEXT(VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
                                      -VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
                                      -VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT),
        (PFN_DebugUtilsMessengerCallbackEXT)notify, nullptr, nullptr);
}
std::string OMRendererVkValidation::attach()
{
    if (enabled)
    {
        return "VK_LAYER_KHRONOS_validation";
    }
    else
    {
        return "";
    }
}
OMRendererVkValidation::~OMRendererVkValidation()
{
}
} // namespace openminecraft::renderer::vk::validation