#ifndef OM_TEXTUREATLAS_HPP
#define OM_TEXTUREATLAS_HPP

#include "openminecraft-shell/data/om_identifier.hpp"
#include "openminecraft/log/om_log_common.hpp"
#include "openminecraft/renderer/common/om_renderer_texture.hpp"
#include "openminecraft/renderer/om_renderer_layer.hpp"
#include "openminecraft/specs/png/om_png.hpp"
#include "openminecraft/vfs/om_vfs_base.hpp"
#include <iostream>
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
        if (!texture)
        {
            delete texture;
        }
    }

    auto addTexture(OMIdentifier i) -> int
    {
        if (subtex.count(i))
        {
            return subtex[i];
        }
        logger.debug("texture {}:{} => {}", i.namesp, i.path, tid);
        subtex[i] = tid;
        ++tid;
        return subtex[i];
    }

    void build()
    {
        texture = renderer->allocateTexture(16, 16, subtex.size(), 4, renderer::common::Dim2Array,
                                            openminecraft::renderer::common::ColorRgba);
        for (auto &p : subtex)
        {
            auto imgraw = vfs::fsfetch(fmt::format("{}/{}/textures/{}.png", root, p.first.namesp, p.first.path));
            specs::png::OMPngFile img2;
            img2.parse(imgraw);
            texture->updateData(img2.fetchData(), p.second);
        }
        texture->mipFilter = openminecraft::renderer::common::Nearest;
        texture->magFilter = openminecraft::renderer::common::Nearest;
        texture->minFilter = openminecraft::renderer::common::Nearest;
        texture->setupSampler();
    }

  private:
    int tid = 0;
    std::string root;
    renderer::OMRenderer *renderer;

  public:
    renderer::common::OMRendererTexture *texture = nullptr;
    std::unordered_map<OMIdentifier, int> subtex;
    log::OMLogger logger;
};
} // namespace openminecraftshell::data

#endif