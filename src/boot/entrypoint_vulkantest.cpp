#include "SDL3/SDL.h"
#include "openminecraft/boot/entrypoint_testrenderer.hpp"
#include "openminecraft/boot/om_boot.hpp"
#include "openminecraft/log/om_log_common.hpp"
#include "openminecraft/renderer/common/om_renderer_shadercompiler.hpp"
#include "openminecraft/renderer/common/shader/om_renderer_shadercompiler_shaderc.hpp"
#include "openminecraft/renderer/om_renderer_layer.hpp"
#include "openminecraft/renderer/vk/om_renderer_layer_vk.hpp"
#include <memory>

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

        bool wk = false, ak = false, sk = false, dk = false, spk = false, lshk = false;

        while (true)
        {
            SDL_Event e;
            SDL_PollEvent(&e);

            if (e.type == SDL_EVENT_KEY_DOWN)
            {
                if (e.key.key == SDLK_W)
                {
                    wk = true;
                }
                else if (e.key.key == SDLK_A)
                {
                    ak = true;
                }
                else if (e.key.key == SDLK_S)
                {
                    sk = true;
                }
                else if (e.key.key == SDLK_D)
                {
                    dk = true;
                }
                else if (e.key.key == SDLK_LSHIFT)
                {
                    lshk = true;
                }
                else if (e.key.key == SDLK_SPACE)
                {
                    spk = true;
                }
                else if (e.key.key == SDLK_ESCAPE)
                {
                    SDL_SetWindowRelativeMouseMode(wnd, false);
                }
            }

            if (e.type == SDL_EVENT_KEY_UP)
            {
                if (e.key.key == SDLK_W)
                {
                    wk = false;
                }
                else if (e.key.key == SDLK_A)
                {
                    ak = false;
                }
                else if (e.key.key == SDLK_S)
                {
                    sk = false;
                }
                else if (e.key.key == SDLK_D)
                {
                    dk = false;
                }
                else if (e.key.key == SDLK_LSHIFT)
                {
                    lshk = false;
                }
                else if (e.key.key == SDLK_SPACE)
                {
                    spk = false;
                }
            }

            if (e.type == SDL_EVENT_WINDOW_RESIZED)
            {
                renderer->requestResize();
            }

            if (e.type == SDL_EVENT_MOUSE_BUTTON_DOWN)
            {
                SDL_SetWindowRelativeMouseMode(wnd, true);
            }

            if (e.type == SDL_EVENT_MOUSE_MOTION && SDL_GetWindowRelativeMouseMode(wnd))
            {
                int ww, hh;
                SDL_GetWindowSize(wnd, &ww, &hh);
                hnd->mouseOffset(e.motion.xrel / ww, e.motion.yrel / hh);
            }

            if (e.type == SDL_EVENT_QUIT)
            {
                break;
            }

            hnd->keyInput(wk, ak, sk, dk, lshk, spk);
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
