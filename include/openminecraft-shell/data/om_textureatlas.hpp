#ifndef OM_TEXTUREATLAS_HPP
#define OM_TEXTUREATLAS_HPP

#include "fmt/format.h"
#include "glm/fwd.hpp"
#include "openminecraft-shell/data/om_identifier.hpp"
#include "openminecraft/log/om_log_common.hpp"
#include "openminecraft/renderer/common/om_renderer_texture.hpp"
#include "openminecraft/renderer/om_renderer_layer.hpp"
#include "openminecraft/specs/png/om_png.hpp"
#include "openminecraft/vfs/om_vfs_base.hpp"
#include <array>
#include <iostream>
#include <tuple>
#include <unordered_map>
#include <utility>
using namespace openminecraft;
namespace openminecraftshell::data
{
class OMTextureAtlas
{
  public:
    OMTextureAtlas(std::string root, renderer::OMRenderer *renderer)
        : renderer(renderer), root(std::move(root)), logger("OMTextureAtlas", this)
    {
    }
    ~OMTextureAtlas()
    {
        delete texture;
    }

    auto addTexture(OMIdentifier i) -> int
    {
        if (subtex.count(i))
        {
            return subtex[i];
        }
        auto imgraw = vfs::fsfetch(fmt::format("{}/{}/textures/{}.png", root, i.namesp, i.path));
        specs::png::OMPngFile img2;
        img2.parse(imgraw);

        int pres = img2.getWidth() / 16 * img2.getHeight() / 16;

        logger.debug("{}texture {}:{} => {}", pres == 1 ? "" : "*", i.namesp, i.path, tid);

        subtex[i] = tid;
        subtexSizes[i] = {img2.getWidth(), img2.getHeight()};
        tid += pres;
        return subtex[i];
    }

    void build()
    {
        texture = renderer->allocateTexture(16, 16, tid, 4, renderer::common::Dim2Array,
                                            openminecraft::renderer::common::ColorRgba);
        for (auto &p : subtex)
        {
            auto imgraw = vfs::fsfetch(fmt::format("{}/{}/textures/{}.png", root, p.first.namesp, p.first.path));
            specs::png::OMPngFile img2;
            img2.parse(imgraw);

            std::array<uint8_t, 4 * 16 * 16> bm = {};
            int texoffset = 0;
            for (int y = 0; y < img2.getHeight() / 16; ++y)
            {
                for (int x = 0; x < img2.getWidth() / 16; ++x)
                {
                    for (int py = 0; py < 16; ++py)
                    {
                        int cx = x * 16;
                        int cy = y * 16 + py;
                        int pixoff = 4 * (cy * img2.getWidth() + cx);

                        std::memcpy(bm.data() + 4 * 16 * py,
                                    reinterpret_cast<void *>(reinterpret_cast<uintptr_t>(img2.fetchData()) + pixoff),
                                    4 * 16);
                    }
                    texture->updateData(bm.data(), p.second + y * img2.getHeight() / 16 + x);
                }
            }
        }
        texture->mipFilter = openminecraft::renderer::common::Nearest;
        texture->magFilter = openminecraft::renderer::common::Nearest;
        texture->minFilter = openminecraft::renderer::common::Nearest;
        texture->setupSampler();
    }

    auto mapTexture(OMIdentifier i, glm::vec2 uv0, glm::vec2 uv1) -> std::tuple<glm::ivec2, glm::ivec2, int>
    {
        uv0 /= 16.0f;
        uv0 *= subtexSizes[i];
        uv1 /= 16.0f;
        uv1 *= subtexSizes[i];

        int offsetx = 0;
        int offsety = 0;

        while (uv0.x > 16 || uv1.x > 16)
        {
            ++offsetx;
            uv0.x -= 16;
            uv1.x -= 16;
        }

        while (uv0.y > 16 || uv1.y > 16)
        {
            ++offsety;
            uv0.y -= 16;
            uv1.y -= 16;
        }

        int texoffset = offsety * subtexSizes[i].y / 16 + offsetx;

        return std::make_tuple(uv0, uv1, subtex[i] + texoffset);
    }

  private:
    int tid = 0;
    std::string root;
    renderer::OMRenderer *renderer;

  public:
    renderer::common::OMRendererTexture *texture = nullptr;
    std::unordered_map<OMIdentifier, int> subtex;
    std::unordered_map<OMIdentifier, glm::ivec2> subtexSizes;
    log::OMLogger logger;
};
} // namespace openminecraftshell::data

#endif
