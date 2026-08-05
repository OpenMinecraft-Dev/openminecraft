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
        delete depthTexture;
        delete target;
    }

    void construct(glm::vec2 ext)
    {
        if (target)
        {
            delete colorTexture;
            delete depthTexture;
        }

        colorTexture = renderer->allocateTexture(ext.x, ext.y, Dim2, ColorRgba);
        depthTexture = renderer->allocateTexture(ext.x, ext.y, Dim2, Depth);

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

    OMRendererRenderTarget *target = nullptr;
    OMRendererTexture *colorTexture;
    OMRendererTexture *depthTexture;
    OMRenderer *renderer;
};
} // namespace openminecraft::renderer::common::wrap

#endif
