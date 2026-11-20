#ifndef OM_ZLIB_DEFLATERSTREAM_HPP
#define OM_ZLIB_DEFLATERSTREAM_HPP

#include "fmt/format.h"
#include <ios>
#include <iostream>
#include <memory>
#include <ostream>
#include <streambuf>
#include <utility>
#include <vector>
#include <stdexcept>
#include <zlib.h>

namespace openminecraft::specs::zlib
{
class OMZlibDeflateStreamBuf : public std::streambuf
{
  public:
    OMZlibDeflateStreamBuf(std::shared_ptr<std::ostream> sink, int windowBits = 15, std::size_t inBufSize = 4096,
                           std::size_t outBufSize = 4096)
        : dst(std::move(sink)), inBuf(inBufSize), outBuf(outBufSize)
    {
        strm.zalloc = Z_NULL;
        strm.zfree = Z_NULL;
        strm.opaque = Z_NULL;
        strm.avail_in = 0;
        strm.next_in = nullptr;
        strm.avail_out = 0;
        strm.next_out = nullptr;

        if (deflateInit2(&strm, Z_DEFAULT_COMPRESSION, Z_DEFLATED, windowBits, 8, Z_DEFAULT_STRATEGY) != Z_OK)
        {
            throw std::runtime_error("deflateInit2 failed");
        }

        char *base = inBuf.data();
        setp(base, base + inBuf.size());
    }

    ~OMZlibDeflateStreamBuf() override
    {
        try
        {
            finish();
        }
        catch (...)
        {
        }
        deflateEnd(&strm);
    }

    void finish()
    {
        if (finished)
        {
            return;
        }

        finished = true;
        deflateChunk(Z_FINISH);
    }

  protected:
    auto overflow(int_type ch) -> int_type override
    {
        deflateChunk(Z_NO_FLUSH);

        if (!traits_type::eq_int_type(ch, traits_type::eof()))
        {
            *pptr() = traits_type::to_char_type(ch);
            pbump(1);
        }

        return traits_type::not_eof(ch);
    }

    auto sync() -> int override
    {
        deflateChunk(Z_SYNC_FLUSH);
        return 0;
    }

  private:
    void deflateChunk(int flush)
    {
        std::size_t pending = static_cast<std::size_t>(pptr() - pbase());

        strm.next_in = reinterpret_cast<Bytef *>(inBuf.data());
        strm.avail_in = static_cast<uInt>(pending);

        do
        {
            strm.next_out = reinterpret_cast<Bytef *>(outBuf.data());
            strm.avail_out = static_cast<uInt>(outBuf.size());

            int ret = deflate(&strm, flush);

            if (ret != Z_OK && ret != Z_STREAM_END && ret != Z_BUF_ERROR)
            {
                throw std::runtime_error(fmt::format("deflate failed, {}", ret));
            }

            std::size_t produced = outBuf.size() - strm.avail_out;

            if (produced > 0)
            {
                dst->write(outBuf.data(), static_cast<std::streamsize>(produced));
            }

            if (ret == Z_STREAM_END)
            {
                break;
            }
        } while (strm.avail_in > 0 || strm.avail_out == 0);

        setp(inBuf.data(), inBuf.data() + inBuf.size());
    }

    std::shared_ptr<std::ostream> dst;
    z_stream strm;
    std::vector<char> inBuf;
    std::vector<char> outBuf;
    bool finished{};
};

class OMZlibDeflaterStream : public std::ostream
{
  public:
    OMZlibDeflaterStream(std::shared_ptr<std::ostream> sink, int windowBits = 15, std::size_t inBufSize = 4096,
                         std::size_t outBufSize = 4096)
        : std::ostream(&buf), buf(sink, windowBits, inBufSize, outBufSize)
    {
    }

    void finish()
    {
        buf.finish();
        flush();
    }

  private:
    OMZlibDeflateStreamBuf buf;
};
} // namespace openminecraft::specs::zlib

#endif
