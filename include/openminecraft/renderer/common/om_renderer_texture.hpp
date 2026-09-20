#ifndef OM_RENDERER_TEXTURE_HPP
#define OM_RENDERER_TEXTURE_HPP
#include "openminecraft/log/om_log_common.hpp"
#include "openminecraft/renderer/om_renderer_object.hpp"

#include <cstdint>

namespace openminecraft::renderer
{
class OMRenderer;
}
namespace openminecraft::renderer::common
{
enum OMTextureType : uint8_t
{
    Dim2,
    Dim2Array,
    Dim2Multisample
};

enum OMTextureArrangement
{
    D32Sfloat,
    D32SfloatS8Uint,
    D24SfloatS8Uint,
    R8G8B8A8Srgb,
    R8G8B8Srgb,
    R8Sint,
    R8G8Sint,
    R8G8B8Sint,
    R8G8B8A8Sint,
    R8Uint,
    R8G8Uint,
    R8G8B8Uint,
    R8G8B8A8Uint,
    R8Snorm,
    R8G8Snorm,
    R8G8B8Snorm,
    R8G8B8A8Snorm,
    R8Unorm,
    R8G8Unorm,
    R8G8B8Unorm,
    R8G8B8A8Unorm,
    R16Sfloat,
    R16G16Sfloat,
    R16G16B16Sfloat,
    R16G16B16A16Sfloat,
    R16Sint,
    R16G16Sint,
    R16G16B16Sint,
    R16G16B16A16Sint,
    R16Uint,
    R16G16Uint,
    R16G16B16Uint,
    R16G16B16A16Uint,
    R16Snorm,
    R16G16Snorm,
    R16G16B16Snorm,
    R16G16B16A16Snorm,
    R16Unorm,
    R16G16Unorm,
    R16G16B16Unorm,
    R16G16B16A16Unorm,
    R32Sfloat,
    R32G32Sfloat,
    R32G32B32Sfloat,
    R32G32B32A32Sfloat,
    R32Sint,
    R32G32Sint,
    R32G32B32Sint,
    R32G32B32A32Sint,
    R32Uint,
    R32G32Uint,
    R32G32B32Uint,
    R32G32B32A32Uint,
};

static inline auto isColorFormat(OMTextureArrangement a) -> bool
{
    return a != D32Sfloat && a != D32SfloatS8Uint && a != D24SfloatS8Uint;
}

enum OMTextureAddressMode
{
    Repeat,
    ClampToBorder,
    ClampToEdge
};

enum OMTextureBorder
{
    OpaqueBlack,
    OpaqueWhite,
    TransparentBlack
};

enum OMTextureFilter : uint8_t
{
    Linear = 0,
    Nearest = 1
};

class OMRendererTexture : public OMRendererObject
{
  public:
    OMRendererTexture(uint64_t width, uint64_t height, uint64_t layers, uint64_t mipmap, OMTextureType type,
                      OMTextureArrangement arr, OMRenderer *renderer);
    virtual ~OMRendererTexture();
    const uint64_t width, height;
    const OMTextureType type;
    const OMTextureArrangement arr;

    virtual void updateData(void *p, uint64_t layer = 0) = 0;
    virtual void updateDataPart(void *p, uint64_t x, uint64_t y, uint64_t w, uint64_t h, uint64_t layer = 0) = 0;

    inline auto objType() -> OMRendererObjectType override
    {
        return Texture;
    }

    virtual void setupSampler() = 0;

    OMTextureAddressMode addressModeU = ClampToEdge;
    OMTextureAddressMode addressModeV = ClampToEdge;
    OMTextureBorder border = OpaqueBlack;
    OMTextureFilter magFilter = Linear;
    OMTextureFilter minFilter = Linear;
    OMTextureFilter mipFilter = Linear;

  protected:
    OMRenderer *renderer;

  private:
    log::OMLogger logger;
};
} // namespace openminecraft::renderer::common

#endif
