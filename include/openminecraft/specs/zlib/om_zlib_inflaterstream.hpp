#ifndef OM_ZLIB_INFLATERSTREAM_HPP
#define OM_ZLIB_INFLATERSTREAM_HPP

#include <iostream>
#include <memory>
#include <streambuf>
#include <istream>
#include <vector>
#include <stdexcept>
#include <zlib.h>

namespace openminecraft::specs::zlib
{
class OMZlibInflateStreamBuf : public std::streambuf
{
  public:
    OMZlibInflateStreamBuf(std::istream &src, int windowBits = 15, std::size_t inBufSize = 4096,
                           std::size_t outBufSize = 4096)
        : src_(src), inBuf_(inBufSize), outBuf_(outBufSize)
    {
        strm_.zalloc = Z_NULL;
        strm_.zfree = Z_NULL;
        strm_.opaque = Z_NULL;
        strm_.avail_in = 0;
        strm_.next_in = nullptr;

        if (inflateInit2(&strm_, windowBits) != Z_OK)
        {
            throw std::runtime_error("inflateInit2 failed");
        }

        char *base = outBuf_.data();
        setg(base, base, base);
    }

    ~OMZlibInflateStreamBuf() override
    {
        inflateEnd(&strm_);
    }

  protected:
    auto underflow() -> int_type override
    {
        if (eof_)
        {
            return traits_type::eof();
        }

        char *outPtr = outBuf_.data();
        std::size_t outSize = outBuf_.size();

        strm_.next_out = reinterpret_cast<Bytef *>(outPtr);
        strm_.avail_out = outSize;

        while (strm_.avail_out == outSize)
        {
            if (strm_.avail_in == 0)
            {
                src_.read(inBuf_.data(), inBuf_.size());
                std::streamsize n = src_.gcount();
                if (n == 0)
                {
                    eof_ = true;
                    return traits_type::eof();
                }
                strm_.next_in = reinterpret_cast<Bytef *>(inBuf_.data());
                strm_.avail_in = static_cast<uInt>(n);
            }

            int ret = inflate(&strm_, Z_NO_FLUSH);

            if (ret != Z_OK && ret != Z_STREAM_END)
            {
                throw std::runtime_error("inflate failed");
            }

            if (strm_.avail_out < outSize)
            {
                break;
            }

            if (ret == Z_STREAM_END)
            {
                eof_ = true;
                return traits_type::eof();
            }
        }

        std::size_t produced = outSize - strm_.avail_out;

        setg(outPtr, outPtr, outPtr + produced);

        return traits_type::to_int_type(*gptr());
    }

  private:
    std::istream &src_;
    z_stream strm_;
    std::vector<char> inBuf_;
    std::vector<char> outBuf_;
    bool eof_{};
};

class OMZlibInflaterStream : public std::istream
{
  public:
    OMZlibInflaterStream(std::shared_ptr<std::istream> src, int windowBits = 15, std::size_t inBufSize = 4096,
                         std::size_t outBufSize = 4096)
        : std::istream(&buf_), buf_(*src, windowBits, inBufSize, outBufSize)
    {
    }

  private:
    std::shared_ptr<std::istream> src;
    OMZlibInflateStreamBuf buf_;
};
} // namespace openminecraft::specs::zlib

#endif
