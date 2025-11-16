#include "openminecraft/renderer/common/om_renderer_buffer.hpp"

namespace openminecraft::renderer::common
{
OMRendererBuffer::OMRendererBuffer(OMBufferUsage usage, uint64_t length)
    : usage(usage), length(length), logger("OMRendererBuffer", this)
{
    logger.info("Allocated");
}

OMRendererBuffer::~OMRendererBuffer()
{
    logger.info("Freed");
}
} // namespace openminecraft::renderer::common