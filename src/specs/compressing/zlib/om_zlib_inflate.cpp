#include "openminecraft/specs/zlib/om_zlib_inflate.hpp"
#include "openminecraft/mem/om_mem_allocator.hpp"
#include "zlib.h"
#include <cstring>
#include <iostream>
#include <stdexcept>

namespace openminecraft::specs::zlib
{
OMZLibInflater::OMZLibInflater(std::function<void(uint8_t *, uint64_t)> dataAcceptor) : dataAcceptor(dataAcceptor)
{
    strm.zalloc = [](void *, uInt n, uInt size) -> void * { return mem::allocator::tracedCallocZLib(n, size); };
    strm.zfree = [](void *, void *d) { mem::allocator::tracedFreeZLib(d); };
    strm.opaque = nullptr;

    inflateInit(&strm);
}
OMZLibInflater::~OMZLibInflater()
{
    inflateEnd(&strm);
}

void OMZLibInflater::input(uint8_t *src, uint64_t length)
{
    buffer.resize(length);
    std::memcpy(buffer.data(), src, length);
    strm.next_in = buffer.data();
    strm.avail_in = buffer.size();

begin:
    uint8_t outBuff[1024];
    memset(outBuff, 0, 1024);
    strm.next_out = outBuff;
    strm.avail_out = 1024;

    auto status = inflate(&strm, Z_NO_FLUSH);

    dataAcceptor(outBuff, 1024 - strm.avail_out);

    if (status == Z_BUF_ERROR || strm.avail_in > 0)
    {
        goto begin;
    }
    if (status == Z_OK || status == Z_STREAM_END)
    {
        return;
    }
    throw std::runtime_error("zlib inflate failed");
}
} // namespace openminecraft::specs::zlib
