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

    inline auto objType() -> OMRendererObjectType override
    {
        return Texture;
    }

  protected:
    OMRenderer *renderer;

  private:
    log::OMLogger logger;
};
} // namespace openminecraft::renderer::common

#endif
