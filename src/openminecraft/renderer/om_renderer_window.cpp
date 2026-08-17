#include "openminecraft/renderer/om_renderer_window.hpp"
#include "SDL3/SDL_video.h"
#include "openminecraft/renderer/common/shader/om_renderer_shadercompiler_shaderc.hpp"
#include "openminecraft/renderer/opengl/om_renderer_layer_opengl.hpp"
#include "openminecraft/renderer/vk/om_renderer_layer_vk.hpp"
#include <memory>

namespace openminecraft::renderer
{
OMWindow::OMWindow(OMVersionRequirement requirement, OMWindowConfig config, std::string shaderPath)
    : logger("OMWindow", this)
{
    renderer::AppInfo a = {config.initialTitle, util::Version(1, 0, 0, 0), "OpenMinecraft Engine",
                           util::Version(1, 0, 0, 0), requirement[config.backend]};
    uint64_t additionalFlags = 0;
    switch (config.backend)
    {
    case Vulkan: {
        additionalFlags = SDL_WINDOW_VULKAN | SDL_WINDOW_RESIZABLE | SDL_WINDOW_HIGH_PIXEL_DENSITY;
        break;
    }
    case OpenGL: {
        additionalFlags = SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE | SDL_WINDOW_HIGH_PIXEL_DENSITY;
        break;
    }
    default:
        break;
    }

    window = SDL_CreateWindow(config.initialTitle.c_str(), config.initialWidth, config.initialHeight, additionalFlags);

    switch (config.backend)
    {
    case Vulkan: {
        renderer = new vk::OMRendererVk(a, [](std::vector<std::string>) -> int { return 0; }, window, shaderPath);
        renderer->compiler.install(std::make_shared<renderer::common::OMRendererShaderCompilerBackendShaderc>());
        break;
    }
    case OpenGL: {
        renderer = new opengl::OMRendererOpenGL(a, window, shaderPath);
        SDL_GL_SetSwapInterval(config.vsync);
        break;
    }
    default:
        break;
    }
}
OMWindow::~OMWindow()
{
    delete renderer;
    SDL_DestroyWindow(window);
}
} // namespace openminecraft::renderer
