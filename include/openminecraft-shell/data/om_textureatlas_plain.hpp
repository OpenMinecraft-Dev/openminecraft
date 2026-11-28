#ifndef OM_TEXTUREATLAS_PLAIN_HPP
#define OM_TEXTUREATLAS_PLAIN_HPP

#include <string>
#include <vector>
#include <algorithm>
#include <limits>
#include <unordered_map>
#include "glm/ext/vector_int4.hpp"
#include "openminecraft-shell/data/om_identifier.hpp"
#include "openminecraft/renderer/common/om_renderer_texture.hpp"
#include "openminecraft/renderer/common/om_renderer_object_ref.hpp"
#include "openminecraft/renderer/om_renderer_layer.hpp"
#include "openminecraft/specs/png/om_png.hpp"
#include "openminecraft/vfs/om_vfs_base.hpp"

using namespace openminecraft;
namespace openminecraft::shell::data
{
class OMTextureAtlasPlain
{
  public:
    OMTextureAtlasPlain(std::string root, openminecraft::renderer::OMRenderer *renderer, uint64_t width,
                        uint64_t height)
        : root(std::move(root)), renderer(renderer), logger("OMTextureAtlasPlain", this), width(width), height(height)
    {

        texture = openminecraft::renderer::common::OMRendererTextureRef(renderer, "atlasplain.main");
        renderer->registerTexture(texture.getTag(), width, height, 1, 0, openminecraft::renderer::common::Dim2,
                                  openminecraft::renderer::common::R8G8B8A8Srgb);

        freeRects.push_back({0, 0, static_cast<int>(width), static_cast<int>(height)});
    }
    ~OMTextureAtlasPlain()
    {
    }

    void addTexture(OMIdentifier i)
    {
        auto imgraw = vfs::fsfetch(fmt::format("{}/{}/textures/{}.png", root, i.namesp, i.path));

        texFiles[i] = std::make_shared<specs::png::OMPngFile>();
        texFiles[i]->parseBase(imgraw);

        const int iw = static_cast<int>(texFiles[i]->getWidth());
        const int ih = static_cast<int>(texFiles[i]->getHeight());

        if (iw <= 0 || ih <= 0)
        {
            logger.error(fmt::format("addTexture: image {}/{} is empty", i.namesp, i.path));
            return;
        }

        const int atlasW = static_cast<int>(width);
        const int atlasH = static_cast<int>(height);

        const int needW = iw + static_cast<int>(padding);
        const int needH = ih + static_cast<int>(padding);

        if (needW > atlasW || needH > atlasH)
        {
            logger.error(fmt::format("addTexture: image {}x{} too large for {}x{} atlas", iw, ih, atlasW, atlasH));
            return;
        }

        int bestIdx = -1;
        int bestShortSideFit = std::numeric_limits<int>::max();
        int bestLongSideFit = std::numeric_limits<int>::max();
        int bestAreaFit = std::numeric_limits<int>::max();
        int bestX = 0, bestY = 0;

        for (size_t idx = 0; idx < freeRects.size(); ++idx)
        {
            const auto &fr = freeRects[idx];
            if (fr.w < needW || fr.h < needH)
                continue;

            const int leftoverW = fr.w - needW;
            const int leftoverH = fr.h - needH;
            const int shortSideFit = std::min(leftoverW, leftoverH);
            const int longSideFit = std::max(leftoverW, leftoverH);
            const int areaFit = fr.w * fr.h - needW * needH;

            bool better = false;
            if (shortSideFit < bestShortSideFit)
            {
                better = true;
            }
            else if (shortSideFit == bestShortSideFit && longSideFit < bestLongSideFit)
            {
                better = true;
            }
            else if (shortSideFit == bestShortSideFit && longSideFit == bestLongSideFit && areaFit < bestAreaFit)
            {
                better = true;
            }

            if (better)
            {
                bestShortSideFit = shortSideFit;
                bestLongSideFit = longSideFit;
                bestAreaFit = areaFit;
                bestIdx = static_cast<int>(idx);
                bestX = fr.x;
                bestY = fr.y;
            }
        }

        if (bestIdx < 0)
        {
            logger.error(fmt::format("addTexture: atlas full, cannot place {}x{} ({}/{})", iw, ih, i.namesp, i.path));
            return;
        }

        occupiedRects[i] = glm::ivec4(bestX, bestY, iw, ih);
        logger.debug("allcated {}x{} at {}x{} for {}:{}", iw, ih, bestX, bestY, i.namesp, i.path);

        const AtlasRect placed{bestX, bestY, needW, needH};

        std::vector<AtlasRect> newFreeRects;
        newFreeRects.reserve(freeRects.size() * 2);

        for (const auto &fr : freeRects)
        {
            const bool noOverlap = placed.x >= fr.x + fr.w || placed.x + placed.w <= fr.x || placed.y >= fr.y + fr.h ||
                                   placed.y + placed.h <= fr.y;

            if (noOverlap)
            {
                newFreeRects.push_back(fr);
                continue;
            }

            if (placed.y + placed.h < fr.y + fr.h)
                newFreeRects.push_back({fr.x, placed.y + placed.h, fr.w, fr.y + fr.h - (placed.y + placed.h)});
            if (placed.y > fr.y)
                newFreeRects.push_back({fr.x, fr.y, fr.w, placed.y - fr.y});
            if (placed.x + placed.w < fr.x + fr.w)
                newFreeRects.push_back({placed.x + placed.w, placed.y, fr.x + fr.w - (placed.x + placed.w), placed.h});
            if (placed.x > fr.x)
                newFreeRects.push_back({fr.x, placed.y, placed.x - fr.x, placed.h});
        }

        std::vector<AtlasRect> pruned;
        pruned.reserve(newFreeRects.size());

        for (size_t a = 0; a < newFreeRects.size(); ++a)
        {
            const auto &A = newFreeRects[a];
            bool contained = false;

            for (size_t b = 0; b < newFreeRects.size(); ++b)
            {
                if (a == b)
                    continue;
                const auto &B = newFreeRects[b];

                const bool aInsideB = A.x >= B.x && A.y >= B.y && A.x + A.w <= B.x + B.w && A.y + A.h <= B.y + B.h;
                if (!aInsideB)
                    continue;

                const bool identical = (A.x == B.x && A.y == B.y && A.w == B.w && A.h == B.h);
                if (identical)
                {
                    if (b > a)
                        continue;
                }
                contained = true;
                break;
            }

            if (!contained)
                pruned.push_back(A);
        }

        freeRects = std::move(pruned);
    }

    void upload()
    {
        for (auto p : occupiedRects)
        {
            const auto &id = p.first;
            const auto &rect = p.second;

            const auto &texFile = texFiles.at(id);

            texture->updateDataPart(texFile->fetchData(), rect.x, rect.y, rect.z, rect.w);
        }
    }

    auto queryRect(OMIdentifier ident) -> glm::vec4
    {
        return glm::vec4(occupiedRects[ident]) / glm::vec4(width, height, width, height);
    }
    openminecraft::renderer::common::OMRendererTextureRef texture;

  private:
    struct AtlasRect
    {
        int x, y, w, h;
    };

    std::string root;
    openminecraft::renderer::OMRenderer *renderer;
    log::OMLogger logger;

    std::unordered_map<OMIdentifier, glm::ivec4> occupiedRects;
    std::unordered_map<OMIdentifier, std::shared_ptr<specs::png::OMPngFile>> texFiles;

    std::vector<AtlasRect> freeRects;
    uint32_t padding = 1;

    uint64_t width, height;
};
}; // namespace openminecraft::shell::data

#endif