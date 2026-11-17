#ifndef OM_RENDERER_OBJECT_HPP
#define OM_RENDERER_OBJECT_HPP

namespace openminecraft::renderer
{
enum OMRendererObjectType
{
    Renderer,
    DataBuffer,
    Handler,
    Pipeline,
    ResourceSet,
    RenderTarget,
    Shader,
    SyncPoint,
    Task,
    Texture,
    ValidationLayerVk,
    SwapchainManagerVk
};

class OMRendererObject
{
  public:
    virtual ~OMRendererObject() = default;
    virtual auto objType() -> OMRendererObjectType = 0;
};
} // namespace openminecraft::renderer

#endif
