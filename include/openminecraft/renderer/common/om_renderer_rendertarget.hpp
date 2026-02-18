#ifndef OM_RENDERER_RENDER_TARGET_HPP
#define OM_RENDERER_RENDER_TARGET_HPP

#include "glm/glm.hpp"
#include "openminecraft/renderer/common/om_renderer_texture.hpp"
namespace openminecraft::renderer
{
class OMRenderer;
}
namespace openminecraft::renderer::common
{
class OMRendererRenderTarget
{
  public:
    OMRendererRenderTarget(OMRenderer *renderer)
    {
    }
    virtual ~OMRendererRenderTarget()
    {
    }

    virtual void attachTarget(OMRendererTexture *texture) = 0;
    virtual void replaceTarget(int idx, OMRendererTexture *texture) = 0;
    virtual void rebuild() = 0;
    virtual glm::vec2 fetchSize() = 0;
    virtual void build() = 0;
};
} // namespace openminecraft::renderer::common

#endif
