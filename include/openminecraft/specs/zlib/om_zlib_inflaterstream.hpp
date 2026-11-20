#ifndef OM_ZLIB_INFLATERSTREAM_HPP
#define OM_ZLIB_INFLATERSTREAM_HPP

#include "fmt/format.h"
#include <ios>
#include <iostream>
#include <memory>
#include <streambuf>
#include <istream>
#include <utility>
#include <vector>
#include <stdexcept>
#include <zlib.h>

namespace openminecraft::specs::zlib
{
class OMZlibInflateStreamBuf : public std::streambuf
{
  public:
    OMZlibInflateStreamBuf(std::shared_ptr<std::istream> source, int windowBits = 15, std::size_t inBufSize = 4096,
                           std::size_t outBufSize = 4096)
        : src(std::move(source)), inBuf(inBufSize), outBuf(outBufSize)
    {
        strm.zalloc = Z_NULL;
        strm.zfree = Z_NULL;
        strm.opaque = Z_NULL;
        strm.avail_in = 0;
        strm.next_in = nullptr;

        if (inflateInit2(&strm, windowBits) != Z_OK)
        {
            throw std::runtime_error("inflateInit2 failed");
        }

        char *base = outBuf.data();
        setg(base, base, base);
    }

    ~OMZlibInflateStreamBuf() override
    {
        inflateEnd(&strm);
    }

  protected:
    auto underflow() -> int_type override
    {
        if (eof)
        {
            return traits_type::eof();
        }

        char *outPtr = outBuf.data();
        std::size_t outSize = outBuf.size();

        strm.next_out = reinterpret_cast<Bytef *>(outPtr);
        strm.avail_out = outSize;

        while (strm.avail_out == outSize)
        {
            if (strm.avail_in == 0)
            {
                src->read(inBuf.data(), inBuf.size());
                std::streamsize n = src->gcount();
                if (n == 0)
                {
                    eof = true;
                    return traits_type::eof();
                }
                strm.next_in = reinterpret_cast<Bytef *>(inBuf.data());
                strm.avail_in = static_cast<uInt>(n);
            }

            int ret = inflate(&strm, Z_NO_FLUSH);

            if (ret != Z_OK && ret != Z_STREAM_END)
            {
                throw std::runtime_error(fmt::format("inflate failed, {}", ret));
            }

            if (strm.avail_out < outSize)
            {
                break;
            }

            if (ret == Z_STREAM_END)
            {
                eof = true;
                return traits_type::eof();
            }
        }

        std::size_t produced = outSize - strm.avail_out;

        setg(outPtr, outPtr, outPtr + produced);

        return traits_type::to_int_type(*gptr());
    }

  private:
    std::shared_ptr<std::istream> src;
    z_stream strm;
    std::vector<char> inBuf;
    std::vector<char> outBuf;
    bool eof{};
};

class OMZlibInflaterStream : public std::istream
{
  public:
    OMZlibInflaterStream(std::shared_ptr<std::istream> source, int windowBits = 15, std::size_t inBufSize = 4096,
                         std::size_t outBufSize = 4096)
        : std::istream(&buf), buf(source, windowBits, inBufSize, outBufSize)
    {
    }

  private:
    std::shared_ptr<std::istream> src;
    OMZlibInflateStreamBuf buf;
};
} // namespace openminecraft::specs::zlib

#endif
