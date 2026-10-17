#ifndef OM_ZIP_SUBSTREAM_HPP
#define OM_ZIP_SUBSTREAM_HPP

#include <streambuf>
#include <istream>
#include <stdexcept>
#include <cstdint>
#include <array>

namespace openminecraft::specs::zip
{
class OMZipSubStreamBuf : public std::streambuf
{
  public:
    OMZipSubStreamBuf(std::istream &source, std::streampos startPos, std::streamsize length)
        : src(source), start(startPos), length(length), absolutePos(0)
    {
        savedPos = src.tellg();
        if (savedPos == -1)
            throw std::runtime_error("Source stream does not support tellg/seekg, required for zero-copy sub-stream");

        src.seekg(start);
        if (!src)
            throw std::runtime_error("Failed to seek to sub-stream start position");

        setg(nullptr, nullptr, nullptr);
    }

    ~OMZipSubStreamBuf() override
    {
        if (savedPos != std::streampos(-1))
        {
            src.clear();
            src.seekg(savedPos);
        }
    }

    OMZipSubStreamBuf(const OMZipSubStreamBuf &) = delete;
    auto operator=(const OMZipSubStreamBuf &) -> OMZipSubStreamBuf & = delete;

  protected:
    auto underflow() -> int_type override
    {
        if (absolutePos >= length)
        {
            return traits_type::eof();
        }

        char ch;
        if (!src.get(ch))
        {
            return traits_type::eof();
        }

        ourBuf[0] = ch;
        setg(&ourBuf[0], &ourBuf[0], &ourBuf[1]);
        absolutePos = static_cast<uint64_t>(absolutePos) + 1;
        return traits_type::to_int_type(ch);
    }

    auto seekoff(off_type off, std::ios_base::seekdir dir, std::ios_base::openmode which = std::ios_base::in)
        -> pos_type override
    {
        if (which != std::ios_base::in)
            return {-1};

        std::streampos newAbsolute;
        switch (dir)
        {
        case std::ios_base::beg:
            newAbsolute = off;
            break;
        case std::ios_base::cur:
            newAbsolute = absolutePos + off;
            break;
        case std::ios_base::end:
            newAbsolute = length + off;
            break;
        default:
            return {-1};
        }

        if (newAbsolute < 0 || newAbsolute > length)
            return {-1}; // 越界

        std::streampos newSrcPos = start + newAbsolute;
        if (!src.seekg(newSrcPos))
            return {-1};

        absolutePos = newAbsolute;
        setg(nullptr, nullptr, nullptr);
        return {newAbsolute};
    }

    auto seekpos(pos_type pos, std::ios_base::openmode which = std::ios_base::in) -> pos_type override
    {
        return seekoff(pos, std::ios_base::beg, which);
    }

  private:
    std::istream &src;
    std::streampos start;
    std::streamsize length;
    std::streampos absolutePos;
    std::streampos savedPos;
    std::array<char, 2> ourBuf;
};

class OMZipSubStream : public std::istream
{
  public:
    OMZipSubStream(std::istream &src, std::streampos start, std::streamsize length)
        : std::istream(&buf), buf(src, start, length)
    {
    }

  private:
    OMZipSubStreamBuf buf;
};
} // namespace openminecraft::specs::zip

#endif
