#include "openminecraft/specs/zlib/om_zlib_deflate.hpp"
#include "openminecraft/mem/om_mem_allocator.hpp"
#include "zlib.h"
#include <array>
#include <cstring>
#include <stdexcept>
#include <utility>

namespace openminecraft::specs::zlib
{
OMZLibDeflater::OMZLibDeflater(std::function<void(uint8_t *, uint64_t)> dataAcceptor)
    : dataAcceptor(std::move(dataAcceptor))
{
    strm.zalloc = [](void *, uInt n, uInt size) -> void * { return mem::allocator::tracedCallocZLib(n, size); };
    strm.zfree = [](void *, void *d) -> void { mem::allocator::tracedFreeZLib(d); };
    strm.opaque = nullptr;

    if (deflateInit(&strm, Z_DEFAULT_COMPRESSION) != Z_OK)
    {
        throw std::runtime_error("zlib deflate init failed");
    }
}
OMZLibDeflater::~OMZLibDeflater()
{
    deflateEnd(&strm);
}

void OMZLibDeflater::input(uint8_t *src, uint64_t length)
{
    buffer.resize(length);
    std::memcpy(buffer.data(), src, length);
    strm.next_in = buffer.data();
    strm.avail_in = buffer.size();

    do
    {
        std::array<uint8_t, 1024> outBuff;
        memset(outBuff.data(), 0, 1024);
        strm.next_out = outBuff.data();
        strm.avail_out = 1024;

        auto status = deflate(&strm, Z_NO_FLUSH);

        dataAcceptor(outBuff.data(), 1024 - strm.avail_out);

        if (status == Z_STREAM_ERROR)
        {
            throw std::runtime_error("zlib deflate failed");
        }
    } while (strm.avail_in > 0);
}

void OMZLibDeflater::finish()
{
    strm.next_in = nullptr;
    strm.avail_in = 0;

    while (true)
    {
        std::array<uint8_t, 1024> outBuff;
        memset(outBuff.data(), 0, 1024);
        strm.next_out = outBuff.data();
        strm.avail_out = outBuff.size();

        auto status = deflate(&strm, Z_FINISH);
        std::size_t produced = 1024 - strm.avail_out;
        if (produced > 0)
        {
            dataAcceptor(outBuff.data(), produced);
        }

        if (status == Z_STREAM_END)
            break;
        if (status != Z_OK && status != Z_BUF_ERROR)
        {
            throw std::runtime_error("zlib deflate finish failed");
        }
    }
}
} // namespace openminecraft::specs::zlib
