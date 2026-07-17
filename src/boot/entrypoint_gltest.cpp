#include "SDL3/SDL_events.h"
#include "SDL3/SDL_video.h"
#include "openminecraft/boot/entrypoint_testrenderer.hpp"
#include "openminecraft/boot/om_boot.hpp"
#include "openminecraft/renderer/opengl/om_renderer_layer_opengl.hpp"
#include "openminecraft/util/om_util_version.hpp"
#include <memory>
#include <thread>
#include "SDL3/SDL.h"

namespace openminecraft::boot
{
void openglRendererTest()
{
    auto logger = std::make_shared<log::OMLogger>("Test OpenGL");
    try
    {
        renderer::AppInfo a = {"OpenMinecraft", util::Version(1, 0, 0, 0), "OpenMinecraft Engine",
                               util::Version(1, 0, 0, 0), util::Version(3, 3, 0, 0)};
        auto wnd = SDL_CreateWindow("OpenGL Test", 800, 800, SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE);
        auto renderer = new renderer::opengl::OMRendererOpenGL(a, wnd);

        SDL_GL_SetSwapInterval(1);

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
        if (!SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "OpenGL Debugger", e.what(), nullptr))
        {
            logger->info("SDL Status: {}", SDL_GetError());
        }
    }
}
} // namespace openminecraft::boot
