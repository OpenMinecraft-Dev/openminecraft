#ifndef OM_RENDERER_WINDOW_HPP
#define OM_RENDERER_WINDOW_HPP

#include "SDL3/SDL_video.h"
#include "openminecraft/renderer/om_renderer_layer.hpp"
#include "openminecraft/util/om_util_version.hpp"
#include <array>
#include <functional>
#include <vector>
namespace openminecraft::renderer
{
enum OMBackend : uint8_t
{
    OpenGL = 0,
    Vulkan = 1
};
using OMVersionRequirement = std::array<util::Version, 2>;
struct OMWindowConfig
{
    OMBackend backend;
    bool vsync = true;
    int maxFps = 240;

    int initialWidth = 800;
    int initialHeight = 600;
    std::string initialTitle = "OpenMinecraft";
    std::function<int(std::vector<std::string>)> selector = [](std::vector<std::string>) -> int { return 0; };
};
class OMWindow
{
  public:
    OMWindow(OMVersionRequirement requirement, OMWindowConfig config, std::string shaderPath);
    ~OMWindow();

    auto operator()() -> OMRenderer *
    {
        return renderer;
    }
    auto operator*() -> void *
    {
        return window;
    }

  private:
    OMVersionRequirement requirement;
    OMWindowConfig config;
    OMRenderer *renderer;
    SDL_Window *window;
};
} // namespace openminecraft::renderer

#endif
