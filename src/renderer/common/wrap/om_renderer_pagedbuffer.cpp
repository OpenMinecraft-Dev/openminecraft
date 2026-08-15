#include "openminecraft/renderer/common/wrap/om_renderer_pagedbuffer.hpp"

namespace openminecraft::renderer::common::wrap
{
OMPagedBuffer::OMPagedBuffer(OMRenderer *renderer, uint32_t pageSize) : renderer(renderer), pageSize(pageSize)
{
    buffer = renderer->allocateBuffer(InstanceData, pageSize * 8);
    occupied.resize(8, false);
}

OMPagedBuffer::~OMPagedBuffer()
{
    delete buffer;
}

auto OMPagedBuffer::expand() -> void
{
    auto nbuffer = renderer->allocateBuffer(InstanceData, buffer->length * 2);
    buffer->copyTo(nbuffer);
    delete buffer;
    buffer = nbuffer;
    occupied.resize(occupied.size() * 2);
}

auto OMPagedBuffer::allocatePage() -> uint32_t
{
begin:
    for (int i = 0; i < occupied.size(); ++i)
    {
        if (!occupied[i])
        {
            occupied[i] = true;
            return i;
        }
    }

    expand();
    goto begin;
}
auto OMPagedBuffer::freePage(uint32_t pageIndex) -> void
{
    if (pageIndex >= 0 && pageIndex < occupied.size())
    {
        occupied[pageIndex] = false;
        onPageFreed(this, pageIndex);
    }
}
} // namespace openminecraft::renderer::common::wrap