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

        if (!target)
        {
            target = renderer->createRenderTarget();
            target->clearDepth = clearDepth;
            target->attachTarget(colorTexture);
            target->attachTarget(depthTexture);
            target->build();
        }
        else
        {
            target->replaceTarget(0, colorTexture);
            target->replaceTarget(1, depthTexture);
            target->rebuild();
        }
    }

    void constructWithDepth(OMRendererTexture *depth, glm::vec2 ext, uint64_t samples = 1, bool flt = false)
    {
        if (target)
        {
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

        if (!target)
        {
            target = renderer->createRenderTarget();
            target->clearDepth = clearDepth;
            target->attachTarget(colorTexture);
            target->attachTarget(depthTexture);
            target->build();
        }
        else
        {
            target->replaceTarget(0, colorTexture);
            target->replaceTarget(1, depthTexture);
            target->rebuild();
        }
    }

    bool externalDepth = false;
    bool clearDepth = true;
    OMRendererRenderTarget *target = nullptr;
    OMRendererTexture *colorTexture;
    OMRendererTexture *depthTexture;
    OMRenderer *renderer;
};
} // namespace openminecraft::renderer::common::wrap

#endif
