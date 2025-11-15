#include "openminecraft/renderer/common/om_renderer_buffer.hpp"

namespace openminecraft::renderer::common
{
OMRendererBuffer::OMRendererBuffer(OMBufferUsage usage, std::function<void *(OMRendererBuffer *)> alloc,
                                   std::function<void(OMRendererBuffer *)> free)
    : usage(usage), free(free), actualBuffer(alloc(this))
{
}

OMRendererBuffer::~OMRendererBuffer()
{
    this->free(this);
}
} // namespace openminecraft::renderer::common