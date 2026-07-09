#include "openminecraft/specs/zlib/om_zlib_inflate.hpp"
#include "openminecraft/mem/om_mem_allocator.hpp"
#include "zlib.h"
#include <array>
#include <cstring>
#include <stdexcept>
#include <utility>

namespace openminecraft::specs::zlib
{
OMZLibInflater::OMZLibInflater(std::function<void(uint8_t *, uint64_t)> dataAcceptor)
    : dataAcceptor(std::move(dataAcceptor))
{
    strm.zalloc = [](void *, uInt n, uInt size) -> void * { return mem::allocator::tracedCallocZLib(n, size); };
    strm.zfree = [](void *, void *d) -> void { mem::allocator::tracedFreeZLib(d); };
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
    std::array<uint8_t, 1024> outBuff;
    memset(outBuff.data(), 0, 1024);
    strm.next_out = outBuff.data();
    strm.avail_out = 1024;

    auto status = inflate(&strm, Z_NO_FLUSH);

    dataAcceptor(outBuff.data(), 1024 - strm.avail_out);

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
