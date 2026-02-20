#include "SDL3/SDL.h"
#include "SDL3/SDL_events.h"
#include "SDL3/SDL_video.h"
#include "openminecraft/boot/entrypoint_testrenderer.hpp"
#include "openminecraft/boot/om_boot.hpp"
#include "openminecraft/renderer/opengl/om_renderer_layer_opengl.hpp"
#include "openminecraft/util/om_util_version.hpp"
#include <memory>

namespace openminecraft::boot
{
void openglRendererTest()
{
    renderer::AppInfo a = {"OpenMinecraft", util::Version(1, 0, 0, 0), "OpenMinecraft Engine",
                           util::Version(1, 0, 0, 0), util::Version(3, 3, 0, 0)};

    auto wnd2 = SDL_CreateWindow("OpenGL Test", 800, 800, SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE);
    auto renderer = new renderer::opengl::OMRendererOpenGL(a, wnd2);
    auto ll = std::make_shared<test::OMTestRenderer>(renderer);

    renderer->registerHandler(ll);
    renderer->baseInit();

    while (true)
    {
        SDL_Event e;
        SDL_PollEvent(&e);

        if (e.type == SDL_EVENT_QUIT)
        {
            break;
        }

        renderer->render();
    }

    ll = nullptr;

    delete renderer;
    SDL_DestroyWindow(wnd2);
}
} // namespace openminecraft::boot
