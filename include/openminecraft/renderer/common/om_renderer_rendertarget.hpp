#ifndef OM_RENDERER_RENDER_TARGET_HPP
#define OM_RENDERER_RENDER_TARGET_HPP

#include "glm/glm.hpp"
#include "openminecraft/renderer/common/om_renderer_texture.hpp"
#include "openminecraft/renderer/om_renderer_object.hpp"
namespace openminecraft::renderer
{
class OMRenderer;
}
namespace openminecraft::renderer::common
{
class OMRendererRenderTarget : public OMRendererObject
{
  public:
    OMRendererRenderTarget(OMRenderer *renderer)
    {
    }
    virtual ~OMRendererRenderTarget() = default;

    virtual void attachTarget(OMRendererTexture *texture) = 0;
    virtual void replaceTarget(int idx, OMRendererTexture *texture) = 0;
    virtual void rebuild() = 0;
    virtual auto fetchSize() -> glm::vec2 = 0;
    virtual void build() = 0;

    inline auto objType() -> OMRendererObjectType override
    {
        return RenderTarget;
    }
};
} // namespace openminecraft::renderer::common

#endif
