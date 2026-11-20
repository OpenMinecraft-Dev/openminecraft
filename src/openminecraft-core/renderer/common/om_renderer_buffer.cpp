#include "openminecraft/renderer/common/om_renderer_buffer.hpp"

namespace openminecraft::renderer::common
{
OMRendererBuffer::OMRendererBuffer(OMBufferUsage usage, uint64_t length, OMRenderer *renderer)
    : usage(usage), length(length), renderer(renderer), logger("OMRendererBuffer", this)
{
    logger.info("{} bytes of buffer ({}) Allocated", length, usage);
}

OMRendererBuffer::~OMRendererBuffer()
{
    logger.info("{} bytes Freed", length);
}
} // namespace openminecraft::renderer::common
