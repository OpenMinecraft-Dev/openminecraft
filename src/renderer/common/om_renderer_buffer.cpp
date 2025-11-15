#include "openminecraft/renderer/common/om_renderer_buffer.hpp"

namespace openminecraft::renderer::common
{
OMRendererBuffer::OMRendererBuffer(OMBufferUsage usage, std::function<void *(OMRendererBuffer *)> alloc,
                                   std::function<void(OMRendererBuffer *)> free)
    : usage(usage), free(free), actualBuffer(nullptr), logger("OMRendererBuffer", this)
{
    logger.info("Allocated");
}

OMRendererBuffer::~OMRendererBuffer()
{
    this->free(this);
    logger.info("Freed");
}
} // namespace openminecraft::renderer::common