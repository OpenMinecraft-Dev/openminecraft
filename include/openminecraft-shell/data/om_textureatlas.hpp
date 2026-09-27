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
#include "openminecraft/io/json/om_io_ast_builder_json.hpp"
#include <array>
#include <memory>
#include <unordered_map>
#include <utility>
#include <vector>
using namespace openminecraft;
namespace openminecraftshell::data
{
struct OMTextureAnimation
{
    int maxFrames;
    int currentFrame = 0;
    bool useMapping;
    std::vector<int> mapping;
    int interval = 0;
    int intervalCount = 0;
};
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

        subtexFiles[i] = std::make_shared<specs::png::OMPngFile>();
        subtexFiles[i]->parse(imgraw);

        subtexSizes[i] = {subtexFiles[i]->getWidth(), subtexFiles[i]->getHeight()};
        if (subtexFiles[i]->getWidth() > 16 && subtexFiles[i]->getHeight() > 16)
        {
            return -1;
        }

        if (subtexFiles[i]->getHeight() > 16)
        {
            subtexAnimData[i] = {subtexFiles[i]->getHeight() / 16};

            auto metadata = vfs::fsfetch(fmt::format("{}/{}/textures/{}.png.mcmeta", root, i.namesp, i.path));
            if (metadata)
            {
                io::json::OMJsonAstBuilder bld(std::make_shared<io::json::OMJsonTokenIter>(metadata));
                auto c = bld.build();
                if (c->getMap().count("animation"))
                {
                    if (c->getMap()["animation"]->getMap().count("frametime"))
                    {
                        subtexAnimData[i].interval = c->getMap()["animation"]->getMap()["frametime"]->getNumber();
                    }
                    if (c->getMap()["animation"]->getMap().count("frames"))
                    {
                        auto ll = c->getMap()["animation"]->getMap()["frames"];
                        int l = 0;
                        for (const auto &fr : ll->getArray())
                        {
                            subtexAnimData[i].mapping.push_back(fr->getNumber());
                            ++l;
                        }
                        subtexAnimData[i].maxFrames = subtexAnimData[i].mapping.size();
                        subtexAnimData[i].useMapping = true;
                    }
                }
            }
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

        subtexWideFiles[i] = std::make_shared<specs::png::OMPngFile>();
        subtexWideFiles[i]->parse(imgraw);

        subtexSizes[i] = {subtexWideFiles[i]->getWidth(), subtexWideFiles[i]->getHeight()};

        if (subtexWideFiles[i]->getHeight() > 32)
        {
            subtexWideAnimData[i] = {subtexWideFiles[i]->getHeight() / 32};

            auto metadata = vfs::fsfetch(fmt::format("{}/{}/textures/{}.png.mcmeta", root, i.namesp, i.path));
            if (metadata)
            {
                io::json::OMJsonAstBuilder bld(std::make_shared<io::json::OMJsonTokenIter>(metadata));
                auto c = bld.build();
                if (c->getMap().count("animation"))
                {
                    if (c->getMap()["animation"]->getMap().count("frametime"))
                    {
                        subtexWideAnimData[i].interval = c->getMap()["animation"]->getMap()["frametime"]->getNumber();
                    }
                    if (c->getMap()["animation"]->getMap().count("frames"))
                    {
                        auto ll = c->getMap()["animation"]->getMap()["frames"];
                        int l = 0;
                        for (const auto &fr : ll->getArray())
                        {
                            subtexWideAnimData[i].mapping.push_back(fr->getNumber());
                            ++l;
                        }
                        subtexWideAnimData[i].maxFrames = subtexWideAnimData[i].mapping.size();
                        subtexWideAnimData[i].useMapping = true;
                    }
                }
            }
        }
        logger.debug("wide texture ({}x{}) {}:{} => {}", subtexSizes[i].x, subtexSizes[i].y, i.namesp, i.path, wtid);

        subtexWide[i] = wtid;
        ++wtid;
        return subtexWide[i];
    }

    void build()
    {
        texture = renderer->allocateTexture(16, 16, tid, 4, openminecraft::renderer::common::Dim2Array,
                                            openminecraft::renderer::common::R8G8B8A8Srgb);
        for (auto &p : subtex)
        {
            if (subtexFiles[p.first]->getWidth() > 16 && subtexFiles[p.first]->getHeight() > 16)
            {
                continue;
            }

            std::array<uint8_t, 4 * 16 * 16> bm = {};
            for (int py = 0; py < 16; ++py)
            {
                int cy = py;
                int pixoff = 4 * (cy * subtexFiles[p.first]->getWidth());

                std::memcpy(
                    bm.data() + 4 * 16 * py,
                    reinterpret_cast<void *>(reinterpret_cast<uintptr_t>(subtexFiles[p.first]->fetchData()) + pixoff),
                    4 * 16);
            }
            texture->updateData(bm.data(), p.second);
        }

        texture->mipFilter = openminecraft::renderer::common::Nearest;
        texture->magFilter = openminecraft::renderer::common::Nearest;
        texture->minFilter = openminecraft::renderer::common::Nearest;
        texture->setupSampler();

        textureSecondary = renderer->allocateTexture(32, 32, wtid, 4, openminecraft::renderer::common::Dim2Array,
                                                     openminecraft::renderer::common::R8G8B8A8Srgb);
        for (auto &p : subtexWide)
        {
            std::array<uint8_t, 4 * 32 * 32> bm = {};
            for (int py = 0; py < 32; ++py)
            {
                int cy = py;
                int pixoff = 4 * (cy * subtexWideFiles[p.first]->getWidth());

                std::memcpy(bm.data() + 4 * 32 * py,
                            reinterpret_cast<void *>(
                                reinterpret_cast<uintptr_t>(subtexWideFiles[p.first]->fetchData()) + pixoff),
                            4 * 32);
            }
            textureSecondary->updateData(bm.data(), p.second);
        }

        textureSecondary->mipFilter = openminecraft::renderer::common::Nearest;
        textureSecondary->magFilter = openminecraft::renderer::common::Nearest;
        textureSecondary->minFilter = openminecraft::renderer::common::Nearest;
        textureSecondary->setupSampler();
    }

    void updateAnim()
    {
        for (auto &p : subtexAnimData)
        {
            if (p.second.interval)
            {
                ++p.second.intervalCount;

                if (p.second.intervalCount >= p.second.interval)
                {
                    p.second.intervalCount = 0;
                }
                else
                {
                    continue;
                }
            }
            p.second.currentFrame = (p.second.currentFrame + 1) % p.second.maxFrames;
            std::array<uint8_t, 4 * 16 * 16> bm = {};
            for (int py = 0; py < 16; ++py)
            {
                int cy =
                    py + (p.second.useMapping ? p.second.mapping[p.second.currentFrame] : p.second.currentFrame) * 16;
                int pixoff = 4 * (cy * subtexFiles[p.first]->getWidth());

                std::memcpy(
                    bm.data() + 4 * 16 * py,
                    reinterpret_cast<void *>(reinterpret_cast<uintptr_t>(subtexFiles[p.first]->fetchData()) + pixoff),
                    4 * 16);
            }
            texture->updateData(bm.data(), subtex[p.first]);
        }

        for (auto &p : subtexWideAnimData)
        {
            if (p.second.interval)
            {
                ++p.second.intervalCount;

                if (p.second.intervalCount >= p.second.interval)
                {
                    p.second.intervalCount = 0;
                }
                else
                {
                    continue;
                }
            }
            p.second.currentFrame = (p.second.currentFrame + 1) % p.second.maxFrames;

            std::array<uint8_t, 4 * 32 * 32> bm = {};
            for (int py = 0; py < 32; ++py)
            {
                int cy =
                    py + (p.second.useMapping ? p.second.mapping[p.second.currentFrame] : p.second.currentFrame) * 32;
                int pixoff = 4 * (cy * subtexWideFiles[p.first]->getWidth());

                std::memcpy(bm.data() + 4 * 32 * py,
                            reinterpret_cast<void *>(
                                reinterpret_cast<uintptr_t>(subtexWideFiles[p.first]->fetchData()) + pixoff),
                            4 * 32);
            }
            textureSecondary->updateData(bm.data(), subtexWide[p.first]);
        }
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
    std::unordered_map<OMIdentifier, std::shared_ptr<specs::png::OMPngFile>> subtexFiles;
    std::unordered_map<OMIdentifier, OMTextureAnimation> subtexAnimData;
    std::unordered_map<OMIdentifier, int> subtexWide;
    std::unordered_map<OMIdentifier, std::shared_ptr<specs::png::OMPngFile>> subtexWideFiles;
    std::unordered_map<OMIdentifier, OMTextureAnimation> subtexWideAnimData;
    std::unordered_map<OMIdentifier, glm::ivec2> subtexSizes;
    log::OMLogger logger;
};
} // namespace openminecraftshell::data

#endif
