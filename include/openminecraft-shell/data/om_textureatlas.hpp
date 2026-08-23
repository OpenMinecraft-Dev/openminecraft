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
#include <unordered_map>
#include <utility>
using namespace openminecraft;
namespace openminecraftshell::data
{
class OMTextureAtlas
{
  public:
    OMTextureAtlas(std::string root, openminecraft::renderer::OMRenderer *renderer)
        : root(std::move(root)), renderer(renderer), logger("OMTextureAtlas", this)
    {
    }
    ~OMTextureAtlas()
    {
        delete textureSecondary;
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

        subtexSizes[i] = {img2.getWidth(), img2.getHeight()};
        if (img2.getWidth() > 16 || img2.getHeight() > 16)
        {
            return -1;
        }
        logger.debug("texture {}:{} => {}", i.namesp, i.path, tid);

        subtex[i] = tid;
        ++tid;
        return subtex[i];
    }

    auto addWideTexture(OMIdentifier i) -> int
    {
        if (subtexWide.count(i))
        {
            return subtexWide[i];
        }
        auto imgraw = vfs::fsfetch(fmt::format("{}/{}/textures/{}.png", root, i.namesp, i.path));
        specs::png::OMPngFile img2;
        img2.parse(imgraw);

        subtexSizes[i] = {img2.getWidth(), img2.getHeight()};
        logger.debug("wide texture ({}x{}) {}:{} => {}", subtexSizes[i].x, subtexSizes[i].y, i.namesp, i.path, wtid);

        subtexWide[i] = wtid;
        ++wtid;
        return subtexWide[i];
    }

    void build()
    {
        texture = renderer->allocateTexture(16, 16, tid, 4, openminecraft::renderer::common::Dim2Array,
                                            openminecraft::renderer::common::ColorRgba);
        for (auto &p : subtex)
        {
            auto imgraw = vfs::fsfetch(fmt::format("{}/{}/textures/{}.png", root, p.first.namesp, p.first.path));
            specs::png::OMPngFile img2;
            img2.parse(imgraw);

            if (img2.getWidth() > 16 || img2.getHeight() > 16)
            {
                continue;
            }

            std::array<uint8_t, 4 * 16 * 16> bm = {};
            for (int py = 0; py < 16; ++py)
            {
                int cy = py;
                int pixoff = 4 * (cy * img2.getWidth());

                std::memcpy(bm.data() + 4 * 16 * py,
                            reinterpret_cast<void *>(reinterpret_cast<uintptr_t>(img2.fetchData()) + pixoff), 4 * 16);
            }
            texture->updateData(bm.data(), p.second);
        }

        texture->mipFilter = openminecraft::renderer::common::Nearest;
        texture->magFilter = openminecraft::renderer::common::Nearest;
        texture->minFilter = openminecraft::renderer::common::Nearest;
        texture->setupSampler();

        textureSecondary = renderer->allocateTexture(32, 32, wtid, 4, openminecraft::renderer::common::Dim2Array,
                                                     openminecraft::renderer::common::ColorRgba);
        for (auto &p : subtexWide)
        {
            auto imgraw = vfs::fsfetch(fmt::format("{}/{}/textures/{}.png", root, p.first.namesp, p.first.path));
            specs::png::OMPngFile img2;
            img2.parse(imgraw);

            std::array<uint8_t, 4 * 32 * 32> bm = {};
            for (int py = 0; py < 32; ++py)
            {
                int cy = py;
                int pixoff = 4 * (cy * img2.getWidth());

                std::memcpy(bm.data() + 4 * 32 * py,
                            reinterpret_cast<void *>(reinterpret_cast<uintptr_t>(img2.fetchData()) + pixoff), 4 * 32);
            }
            textureSecondary->updateData(bm.data(), p.second);
        }

        textureSecondary->mipFilter = openminecraft::renderer::common::Nearest;
        textureSecondary->magFilter = openminecraft::renderer::common::Nearest;
        textureSecondary->minFilter = openminecraft::renderer::common::Nearest;
        textureSecondary->setupSampler();
    }

    auto textureSize(OMIdentifier i) -> glm::ivec2
    {
        return subtexSizes[i];
    }

  private:
    int tid = 0;
    int wtid = 0;
    std::string root;
    openminecraft::renderer::OMRenderer *renderer;

  public:
    openminecraft::renderer::common::OMRendererTexture *texture = nullptr;
    openminecraft::renderer::common::OMRendererTexture *textureSecondary = nullptr;
    std::unordered_map<OMIdentifier, int> subtex;
    std::unordered_map<OMIdentifier, int> subtexWide;
    std::unordered_map<OMIdentifier, glm::ivec2> subtexSizes;
    log::OMLogger logger;
};
} // namespace openminecraftshell::data

#endif
