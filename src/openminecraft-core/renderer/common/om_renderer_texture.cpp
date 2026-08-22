#include "openminecraft/renderer/common/om_renderer_texture.hpp"

namespace openminecraft::renderer::common
{
OMRendererTexture::OMRendererTexture(uint64_t width, uint64_t height, uint64_t layers, uint64_t mipmap,
                                     OMTextureType type, OMTextureArrangement arr, OMRenderer *renderer)
    : width(width), height(height), type(type), arr(arr), renderer(renderer), logger("OMRendererTexture", this)
{
}

OMRendererTexture::~OMRendererTexture() = default;

} // namespace openminecraft::renderer::common
