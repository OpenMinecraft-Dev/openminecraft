#include "SDL3/SDL.h"
#include "openminecraft/boot/entrypoint_testrenderer.hpp"
#include "openminecraft/boot/om_boot.hpp"
#include "openminecraft/log/om_log_common.hpp"
#include "openminecraft/renderer/common/om_renderer_shadercompiler.hpp"
#include "openminecraft/renderer/common/shader/om_renderer_shadercompiler_shaderc.hpp"
#include "openminecraft/renderer/om_renderer_layer.hpp"
#include "openminecraft/renderer/vk/om_renderer_layer_vk.hpp"
#include <memory>
#include <thread>

namespace openminecraft::boot
{
void vulkanRendererTest()
{
    auto logger = std::make_shared<log::OMLogger>("Test Vulkan");
    try
    {
        renderer::AppInfo a = {"OpenMinecraft", util::Version(1, 0, 0, 0), "OpenMinecraft Engine",
                               util::Version(1, 0, 0, 0), util::Version(1, 2, 0, 0)};
        auto wnd = SDL_CreateWindow("Vulkan Test", 800, 800, SDL_WINDOW_VULKAN | SDL_WINDOW_RESIZABLE);
        auto renderer = new renderer::vk::OMRendererVk(a, [](std::vector<std::string>) -> int { return 0; }, wnd);
        renderer->compiler.install(std::make_shared<renderer::common::OMRendererShaderCompilerBackendShaderc>());

        auto hnd = std::make_shared<test::OMTestRenderer>(renderer);
        renderer->registerHandler(hnd);

        renderer->baseInit();

        logger->info("driver: {}", renderer->driver());

        std::thread t([&]() -> void {
            while (true)
            {
                hnd->eventLoop(wnd);
            }
        });

        while (true)
        {
            SDL_Event e;
            SDL_PollEvent(&e);

            if (e.type == SDL_EVENT_WINDOW_RESIZED)
            {
                renderer->requestResize();
            }

            renderer->render();
        }

        hnd = nullptr;
        delete renderer;
        SDL_DestroyWindow(wnd);
    }
    catch (std::runtime_error &e)
    {
        if (!SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Vulkan Debugger", e.what(), nullptr))
        {
            logger->info("SDL Status: {}", SDL_GetError());
        }
    }
}

} // namespace openminecraft::boot
