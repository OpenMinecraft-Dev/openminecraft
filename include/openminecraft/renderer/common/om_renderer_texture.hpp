#ifndef OM_RENDERER_TEXTURE_HPP
#define OM_RENDERER_TEXTURE_HPP
#include "openminecraft/log/om_log_common.hpp"

#include <cstdint>

namespace openminecraft::renderer
{
class OMRenderer;
}
namespace openminecraft::renderer::common
{
enum OMTextureType : uint8_t
{
    Dim1, Dim2, Dim3
};

enum OMTextureArrangement
{
    Depth, ColorRgba, ColorRgb
};

class OMRendererTexture
{
  public:
    OMRendererTexture(uint64_t width, uint64_t height, OMTextureType type, OMTextureArrangement arr, OMRenderer *renderer);
    virtual ~OMRendererTexture();
    const uint64_t width, height;
    const OMTextureType type;
    const OMTextureArrangement arr;

  protected:
    OMRenderer *renderer;

  private:
    log::OMLogger logger;
};
} // namespace openminecraft::renderer::common

#endif