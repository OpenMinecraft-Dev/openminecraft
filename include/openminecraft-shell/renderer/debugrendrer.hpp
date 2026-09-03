#ifndef DEBUGRENDERER_HPP
#define DEBUGRENDERER_HPP

#include "openminecraft/fontproc/om_fontset.hpp"
#include "openminecraft/renderer/common/basics/om_camera.hpp"
#include "openminecraft/renderer/common/demiurge/om_demiurge_node.hpp"
#include "openminecraft/renderer/common/om_renderer_handler.hpp"
#include "openminecraft/renderer/common/demiurge/om_demiurge_rendererhandler.hpp"
#include <array>
#include <cmath>

namespace openminecraftshell::renderer
{
constexpr std::array<int, 16> DebugColors = {
    (int)0xF9FFFFFF, (int)0xF9801DFF, (int)0xC74EBDFF, 0x3AB3DAFF, (int)0xFED83DFF, (int)0x80C71FFF,
    (int)0xF38BAAFF, 0x474F52FF,      (int)0x9D9D97FF, 0x169C9CFF, (int)0x8932B8FF, 0x3C44AAFF,
    (int)0x835432FF, 0x5E7C16FF,      (int)0xB02E26FF, 0x1D1D21FF,
};

class OMDebugRenderer : public openminecraft::renderer::common::OMRendererHandler
{
  public:
    OMDebugRenderer(openminecraft::renderer::OMRenderer *renderer);
    virtual ~OMDebugRenderer() override;

    void submitTasks() override;
    void beforeFrame() override;
    void afterFrame() override;

    auto getUlpf(float s) -> float
    {
        if (s == 0.0)
        {
            return 0.0f;
        }

        int exp;
        std::frexp(s, &exp);
        return std::ldexp(1.0, exp - 1 - 23);
    }
    auto getUlp(double s) -> double
    {
        if (s == 0.0)
        {
            return 0.0;
        }

        int exp;
        std::frexp(s, &exp);
        return std::ldexp(1.0, exp - 1 - 52);
    }

    std::shared_ptr<openminecraft::renderer::common::demiurge::OMDemiurgeNode> node, fpsTextNode, posTextNode,
        povTextNode, precisionNode, precisionNode2;
    std::shared_ptr<openminecraft::renderer::common::demiurge::OMDemiurgeRendererHandler> internal;
    std::shared_ptr<openminecraft::fontproc::OMFontSet> fontset;
    std::vector<std::shared_ptr<openminecraft::renderer::common::demiurge::OMDemiurgeNode>> sectorNodes = {},
                                                                                            textNodes = {};
    int fps = 0;
    std::chrono::steady_clock::time_point tp = {};
    openminecraft::renderer::OMRenderer *renderer;
    openminecraft::renderer::common::basics::OMCamera *camera;
};
} // namespace openminecraftshell::renderer

#endif