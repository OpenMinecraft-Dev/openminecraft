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
    Dim1,
    Dim2,
    Dim3
};

enum OMTextureArrangement
{
    Depth,
    ColorRgba,
    ColorRgb
};

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

class OMRendererTexture : public OMRendererObject
{
  public:
    OMRendererTexture(uint64_t width, uint64_t height, OMTextureType type, OMTextureArrangement arr,
                      OMRenderer *renderer);
    virtual ~OMRendererTexture();
    const uint64_t width, height;
    const OMTextureType type;
    const OMTextureArrangement arr;

    virtual void updateData(void *p) = 0;
    virtual void updateDataPart(void *p, uint64_t x, uint64_t y, uint64_t w, uint64_t h) = 0;

    inline auto objType() -> OMRendererObjectType override
    {
        return Texture;
    }

    OMTextureAddressMode addressMode = ClampToEdge;
    OMTextureBorder border = OpaqueBlack;

  protected:
    OMRenderer *renderer;

  private:
    log::OMLogger logger;
};
} // namespace openminecraft::renderer::common

#endif
