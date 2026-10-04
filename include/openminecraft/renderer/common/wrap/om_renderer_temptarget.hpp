#ifndef OM_RENDERER_TEMPTARGET_HPP
#define OM_RENDERER_TEMPTARGET_HPP

#include "glm/ext/vector_float2.hpp"
#include "openminecraft/renderer/common/om_renderer_rendertarget.hpp"
#include "openminecraft/renderer/common/om_renderer_texture.hpp"
#include "openminecraft/renderer/om_renderer_layer.hpp"
namespace openminecraft::renderer::common::wrap
{
class OMRendererTempTarget
{
  public:
    OMRendererTempTarget(OMRenderer *renderer) : renderer(renderer)
    {
    }
    ~OMRendererTempTarget()
    {
        for (auto tex : additionalTex)
        {
            delete tex;
        }
        additionalTex.clear();
        delete colorTexture;
        if (!externalDepth)
        {
            delete depthTexture;
        }
        delete target;
    }

    void construct(glm::vec2 ext, uint64_t samples = 1, bool flt = false)
    {
        if (target)
        {
            for (auto tex : additionalTex)
            {
                delete tex;
            }
            additionalTex.clear();
            delete colorTexture;
            if (!externalDepth)
            {
                delete depthTexture;
            }
        }

        externalDepth = false;

        if (samples <= 1)
        {
            colorTexture = renderer->allocateTexture(ext.x, ext.y, 0, Dim2, flt ? R16G16B16A16Sfloat : R8G8B8A8Srgb);
            colorTexture->setupSampler();
            depthTexture = renderer->allocateTexture(ext.x, ext.y, 0, Dim2, D32Sfloat);
        }
        else
        {
            colorTexture = renderer->allocateTexture(ext.x, ext.y, samples, 0, Dim2Multisample,
                                                     flt ? R16G16B16A16Sfloat : R8G8B8A8Srgb);
            colorTexture->setupSampler();
            depthTexture = renderer->allocateTexture(ext.x, ext.y, samples, 0, Dim2Multisample, D32Sfloat);
        }

        for (auto tf : additionalTexFormats)
        {
            auto tex = renderer->allocateTexture(ext.x, ext.y, samples, 0, samples <= 1 ? Dim2 : Dim2Multisample, tf);
            tex->setupSampler();
            additionalTex.push_back(tex);
        }

        if (!target)
        {
            target = renderer->createRenderTarget();
            target->clearDepth = clearDepth;
            target->storeDepth = storeDepth;
            target->attachTarget(colorTexture);
            for (auto tex : additionalTex)
            {
                target->attachTarget(tex);
            }
            target->attachTarget(depthTexture);
            target->build();
        }
        else
        {
            target->replaceTarget(0, colorTexture);
            int i = 1;
            for (auto tex : additionalTex)
            {
                target->replaceTarget(i, tex);
                i++;
            }
            target->replaceTarget(i, depthTexture);
            target->rebuild();
        }
    }

    void constructWithDepth(OMRendererTexture *depth, glm::vec2 ext, uint64_t samples = 1, bool flt = false)
    {
        if (target)
        {
            for (auto tex : additionalTex)
            {
                delete tex;
            }
            additionalTex.clear();
            delete colorTexture;
            if (!externalDepth)
            {
                delete depthTexture;
            }
        }

        externalDepth = true;
        depthTexture = depth;

        if (samples <= 1)
        {
            colorTexture = renderer->allocateTexture(ext.x, ext.y, 0, Dim2, flt ? R16G16B16A16Sfloat : R8G8B8A8Srgb);
            colorTexture->setupSampler();
        }
        else
        {
            colorTexture = renderer->allocateTexture(ext.x, ext.y, samples, 0, Dim2Multisample,
                                                     flt ? R16G16B16A16Sfloat : R8G8B8A8Srgb);
            colorTexture->setupSampler();
        }

        for (auto tf : additionalTexFormats)
        {
            auto tex = renderer->allocateTexture(ext.x, ext.y, samples, 0, samples <= 1 ? Dim2 : Dim2Multisample, tf);
            tex->setupSampler();
            additionalTex.push_back(tex);
        }

        if (!target)
        {
            target = renderer->createRenderTarget();
            target->clearDepth = clearDepth;
            target->storeDepth = storeDepth;
            target->attachTarget(colorTexture);
            for (auto tex : additionalTex)
            {
                target->attachTarget(tex);
            }
            target->attachTarget(depthTexture);
            target->build();
        }
        else
        {
            target->replaceTarget(0, colorTexture);
            int i = 1;
            for (auto tex : additionalTex)
            {
                target->replaceTarget(i, tex);
                i++;
            }
            target->replaceTarget(i, depthTexture);
            target->rebuild();
        }
    }

    bool externalDepth = false;
    bool clearDepth = true;
    bool storeDepth = false;
    OMRendererRenderTarget *target = nullptr;
    OMRendererTexture *colorTexture;
    OMRendererTexture *depthTexture;
    OMRenderer *renderer;

    std::vector<OMTextureArrangement> additionalTexFormats = {};
    std::vector<OMRendererTexture *> additionalTex = {};
};
} // namespace openminecraft::renderer::common::wrap

#endif
