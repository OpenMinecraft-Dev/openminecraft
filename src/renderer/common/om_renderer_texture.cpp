#include "openminecraft/renderer/common/om_renderer_texture.hpp"

namespace openminecraft::renderer::common
{
OMRendererTexture::OMRendererTexture(uint64_t width, uint64_t height, OMTextureType type, OMTextureArrangement arr, OMRenderer *renderer)
    : width(width), height(height), renderer(renderer), logger("OMRendererTexture", this), type(type), arr(arr)
{
}

OMRendererTexture::~OMRendererTexture()
{
}

} // namespace openminecraft::renderer::common