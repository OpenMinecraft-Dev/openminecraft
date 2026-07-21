#include "openminecraft/boot/entrypoint_testrenderer.hpp"
#include "openminecraft/boot/om_boot.hpp"
#include "openminecraft/log/om_log_common.hpp"
#include "openminecraft/renderer/om_renderer_layer.hpp"
#include "openminecraft/renderer/om_renderer_window.hpp"
#include "openminecraft/util/om_util_version.hpp"
#include "openminecraft/renderer/common/demiurge/om_demiurge_rendererhandler.hpp"
#include <array>
#include <memory>
#include <thread>

namespace openminecraft::boot
{
void rendererTest(renderer::OMBackend backend)
{
    auto logger = std::make_shared<log::OMLogger>("Test Renderer");
    try
    {
        renderer::OMWindowConfig conf = {backend};
        // INFO: requires at least OpenGL 3.3 Core Profile or Vulkan 1.2
        renderer::OMWindow win({util::Version(3, 3, 0, 0), util::Version(1, 2, 0, 0)}, conf);

        auto hnd = std::make_shared<test::OMTestRenderer>(win());
        auto hnd2 = std::make_shared<renderer::common::demiurge::OMDemiurgeRendererHandler>(win());
        win()->registerHandler(hnd);
        win()->registerHandler(hnd2);
        win()->baseInit();

        logger->info("driver: {}", win()->driver());

        // INFO: states[0] represents application status, states[1] represents if the window resized
        std::array<bool, 2> states = {false, false};
        std::thread t([&]() -> void {
            while (!states[0])
            {
                hnd->eventLoop(*win, states.data());
            }
        });

        while (!states[0])
        {
            if (states[1])
            {
                win()->requestResize();
                states[1] = false;
            }

            win()->render();
        }

        t.join();
        hnd = nullptr;
        hnd2 = nullptr;
    }
    catch (std::runtime_error &e)
    {
        logger->fatal(e.what());
    }
}
} // namespace openminecraft::boot
