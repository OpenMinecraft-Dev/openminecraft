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
    createInfo = DebugUtilsMessengerCreateInfoEXT(
        DebugUtilsMessengerCreateFlagsEXT(),
        DebugUtilsMessageSeverityFlagBitsEXT::eVerbose | DebugUtilsMessageSeverityFlagBitsEXT::eInfo |
            DebugUtilsMessageSeverityFlagBitsEXT::eWarning | DebugUtilsMessageSeverityFlagBitsEXT::eError,
        DebugUtilsMessageTypeFlagBitsEXT::eGeneral | DebugUtilsMessageTypeFlagBitsEXT::ePerformance |
            DebugUtilsMessageTypeFlagBitsEXT::eValidation,
        PFN_DebugUtilsMessengerCallbackEXT(notify), nullptr, nullptr);
}
void OMRendererVkValidation::attachExts(std::vector<const char *> *data)
{
    if (enabled)
    {
        data->push_back("VK_EXT_debug_utils");
    }
}
void OMRendererVkValidation::attach(std::vector<const char *> *data)
{
    if (enabled)
    {
        data->push_back("VK_LAYER_KHRONOS_validation");
    }
}
OMRendererVkValidation::~OMRendererVkValidation()
{
}
} // namespace openminecraft::renderer::vk::validation