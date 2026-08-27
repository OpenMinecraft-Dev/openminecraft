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

    void construct(glm::vec2 ext, uint64_t samples = 1)
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
            colorTexture = renderer->allocateTexture(ext.x, ext.y, 0, Dim2, ColorRgbaF16);
            colorTexture->setupSampler();
            depthTexture = renderer->allocateTexture(ext.x, ext.y, 0, Dim2, Depth);
        }
        else
        {
            colorTexture = renderer->allocateTexture(ext.x, ext.y, samples, 0, Dim2Multisample, ColorRgbaF16);
            colorTexture->setupSampler();
            depthTexture = renderer->allocateTexture(ext.x, ext.y, samples, 0, Dim2Multisample, Depth);
        }

        if (!target)
        {
            target = renderer->createRenderTarget();
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

    void constructWithDepth(OMRendererTexture *depth, glm::vec2 ext, uint64_t samples = 1)
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
            colorTexture = renderer->allocateTexture(ext.x, ext.y, 0, Dim2, ColorRgbaF16);
            colorTexture->setupSampler();
        }
        else
        {
            colorTexture = renderer->allocateTexture(ext.x, ext.y, samples, 0, Dim2Multisample, ColorRgbaF16);
            colorTexture->setupSampler();
        }

        if (!target)
        {
            target = renderer->createRenderTarget();
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
    OMRendererRenderTarget *target = nullptr;
    OMRendererTexture *colorTexture;
    OMRendererTexture *depthTexture;
    OMRenderer *renderer;
};
} // namespace openminecraft::renderer::common::wrap

#endif
