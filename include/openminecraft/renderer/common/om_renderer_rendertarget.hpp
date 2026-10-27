#ifndef OM_RENDERER_RENDERTARGET_HPP
#define OM_RENDERER_RENDERTARGET_HPP

#include "glm/glm.hpp"
#include "openminecraft/renderer/common/om_renderer_texture.hpp"
#include "openminecraft/renderer/om_renderer_object.hpp"
#include <initializer_list>
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

    // INFO: 配置项统一走 setter，子类（各后端）可直接读取保护成员
    inline auto setClearDepth(bool value) -> OMRendererRenderTarget *
    {
        clearDepth = value;
        return this;
    }

    inline auto setStoreDepth(bool value) -> OMRendererRenderTarget *
    {
        storeDepth = value;
        return this;
    }

    inline auto addClearColorAtt(bool value) -> OMRendererRenderTarget *
    {
        clearColorAtt.push_back(value);
        return this;
    }

    inline auto isClearDepth() const -> bool
    {
        return clearDepth;
    }

    inline auto isStoreDepth() const -> bool
    {
        return storeDepth;
    }

    inline auto clearColorAttachments() const -> const std::vector<bool> &
    {
        return clearColorAtt;
    }

  protected:
    bool clearDepth = true;
    bool storeDepth = false;
    std::vector<bool> clearColorAtt = {true};
};
} // namespace openminecraft::renderer::common

#endif
