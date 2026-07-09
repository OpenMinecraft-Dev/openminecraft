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
    RenderTarget,
    Shader,
    Task,
    Texture,
    ValidationLayerVk,
    SwapchainManagerVk
};

class OMRendererObject
{
  public:
    virtual auto objType() -> OMRendererObjectType = 0;
};
} // namespace openminecraft::renderer

#endif
