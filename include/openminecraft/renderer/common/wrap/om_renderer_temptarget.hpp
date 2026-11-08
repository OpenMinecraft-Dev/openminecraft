#ifndef OM_RENDERER_TEMPTARGET_HPP
#define OM_RENDERER_TEMPTARGET_HPP

#include "glm/ext/vector_float2.hpp"
#include "openminecraft/renderer/common/om_renderer_rendertarget.hpp"
#include "openminecraft/renderer/common/om_renderer_texture.hpp"
#include "openminecraft/renderer/om_renderer_layer.hpp"
#include <atomic>
#include <string>
#include <vector>
namespace openminecraft::renderer::common::wrap
{
// Temporary render target: all textures / render targets are registered in OMRenderer's registry,
// destroyed by the renderer; this class only holds string tags.
class OMRendererTempTarget
{
  public:
    OMRendererTempTarget(OMRenderer *renderer, std::string tagPrefix)
        : renderer(renderer),
          prefix(std::move(tagPrefix) + "." + std::to_string(++instanceCounter)), targetTag(prefix + ".target")
    {
    }

    ~OMRendererTempTarget() = default;

    void construct(glm::vec2 ext, uint64_t samples = 1, bool flt = false)
    {
        rebuild(ext, samples, flt, {}, false);
    }

    void constructWithDepth(const std::string &depthTag, glm::vec2 ext, uint64_t samples = 1, bool flt = false)
    {
        rebuild(ext, samples, flt, depthTag, true);
    }

    auto target() -> OMRendererRenderTarget *
    {
        return renderer->getRenderTarget(targetTag);
    }

    auto colorTexture() -> OMRendererTexture *
    {
        return renderer->getTexture(colorTag);
    }

    auto depthTexture() -> OMRendererTexture *
    {
        return renderer->getTexture(depthTag);
    }

    auto additionalTex(size_t i) -> OMRendererTexture *
    {
        return renderer->getTexture(additionalTags.at(i));
    }

    auto additionalTexCount() const -> size_t
    {
        return additionalTags.size();
    }

    auto targetTagName() const -> const std::string &
    {
        return targetTag;
    }

    auto colorTexTag() const -> const std::string &
    {
        return colorTag;
    }

    auto depthTexTag() const -> const std::string &
    {
        return depthTag;
    }

    // INFO: configuration goes through setters
    auto setClearDepth(bool value) -> OMRendererTempTarget *
    {
        clearDepth = value;
        return this;
    }

    auto setStoreDepth(bool value) -> OMRendererTempTarget *
    {
        storeDepth = value;
        return this;
    }

    auto isExternalDepth() const -> bool
    {
        return externalDepth;
    }

    OMRenderer *renderer;
    std::vector<OMTextureArrangement> additionalTexFormats = {};

  private:
    void rebuild(glm::vec2 ext, uint64_t samples, bool flt, const std::string &externalDepthTag, bool external)
    {
        const std::string suffix = ".v" + std::to_string(++serial);
        const std::string oldColor = colorTag;
        const std::string oldDepth = depthTag;
        const auto oldAdditional = additionalTags;
        const bool oldExternal = externalDepth;

        externalDepth = external;
        colorTag = prefix + suffix + ".color";
        additionalTags.clear();

        const auto colorFormat = flt ? R16G16B16A16Sfloat : R8G8B8A8Srgb;

        if (samples <= 1)
        {
            auto *color = renderer->registerTexture(colorTag, ext.x, ext.y, 0, Dim2, colorFormat);
            color->addressModeU = ClampToEdge;
            color->addressModeV = ClampToEdge;
            color->setupSampler();

            if (!external)
            {
                depthTag = prefix + suffix + ".depth";
                renderer->registerTexture(depthTag, ext.x, ext.y, 0, Dim2, D32Sfloat);
            }
        }
        else
        {
            auto *color =
                renderer->registerTexture(colorTag, ext.x, ext.y, samples, 0, Dim2Multisample, colorFormat);
            color->addressModeU = ClampToEdge;
            color->addressModeV = ClampToEdge;
            color->setupSampler();

            if (!external)
            {
                depthTag = prefix + suffix + ".depth";
                renderer->registerTexture(depthTag, ext.x, ext.y, samples, 0, Dim2Multisample, D32Sfloat);
            }
        }

        if (external)
        {
            depthTag = externalDepthTag;
        }

        for (size_t i = 0; i < additionalTexFormats.size(); ++i)
        {
            const std::string tag = prefix + suffix + ".add" + std::to_string(i);
            auto *tex = renderer->registerTexture(tag, ext.x, ext.y, samples, 0,
                                                  samples <= 1 ? Dim2 : Dim2Multisample, additionalTexFormats[i]);
            tex->setupSampler();
            additionalTags.push_back(tag);
        }

        if (!renderer->hasRenderTarget(targetTag))
        {
            auto *tgt = renderer->registerRenderTarget(targetTag);
            tgt->setClearDepth(clearDepth)->setStoreDepth(storeDepth);
            tgt->attachTarget(colorTexture());
            for (size_t i = 0; i < additionalTags.size(); ++i)
            {
                tgt->attachTarget(additionalTex(i));
                tgt->addClearColorAtt(true);
            }
            tgt->attachTarget(depthTexture());
            tgt->build();
        }
        else
        {
            auto *tgt = target();
            tgt->replaceTarget(0, colorTexture());
            size_t i = 1;
            for (size_t n = 0; n < additionalTags.size(); ++n)
            {
                tgt->replaceTarget(i, additionalTex(n));
                ++i;
            }
            tgt->replaceTarget(i, depthTexture());
            tgt->rebuild();
        }

        // Old textures are destroyed by the registry (rebuild the target first to avoid references to freed objects)
        if (!oldColor.empty())
        {
            renderer->unregisterTexture(oldColor);
        }

        if (!oldExternal && !oldDepth.empty())
        {
            renderer->unregisterTexture(oldDepth);
        }

        for (const auto &tag : oldAdditional)
        {
            renderer->unregisterTexture(tag);
        }
    }

    std::string prefix;
    std::string targetTag;
    std::string colorTag;
    std::string depthTag;
    std::vector<std::string> additionalTags;
    uint64_t serial = 0;
    // The same prefix may be instantiated multiple times (e.g. several demiurge handlers); append an instance id to keep tags unique
    static inline std::atomic<uint64_t> instanceCounter = 0;
    bool externalDepth = false;
    bool clearDepth = true;
    bool storeDepth = false;
};
} // namespace openminecraft::renderer::common::wrap

#endif
