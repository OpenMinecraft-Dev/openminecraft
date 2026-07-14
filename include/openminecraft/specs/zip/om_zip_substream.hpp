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
    OMZipSubStreamBuf(std::istream &src, std::streampos startPos, std::streamsize length)
        : src_(src), start_(startPos), length_(length), absolutePos_(0)
    {
        savedPos_ = src_.tellg();
        if (savedPos_ == -1)
            throw std::runtime_error("Source stream does not support tellg/seekg, required for zero-copy sub-stream");

        src_.seekg(start_);
        if (!src_)
            throw std::runtime_error("Failed to seek to sub-stream start position");

        setg(nullptr, nullptr, nullptr);
    }

    ~OMZipSubStreamBuf() override
    {
        if (savedPos_ != std::streampos(-1))
        {
            src_.clear();
            src_.seekg(savedPos_);
        }
    }

    OMZipSubStreamBuf(const OMZipSubStreamBuf &) = delete;
    auto operator=(const OMZipSubStreamBuf &) -> OMZipSubStreamBuf & = delete;

  protected:
    auto underflow() -> int_type override
    {
        if (absolutePos_ >= length_)
        {
            return traits_type::eof();
        }

        char ch;
        if (!src_.get(ch))
        {
            return traits_type::eof();
        }

        ourBuf_[0] = ch;
        setg(ourBuf_.begin(), ourBuf_.begin(), ourBuf_.begin() + 1);
        absolutePos_ = static_cast<uint64_t>(absolutePos_) + 1;
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
            newAbsolute = absolutePos_ + off;
            break;
        case std::ios_base::end:
            newAbsolute = length_ + off;
            break;
        default:
            return {-1};
        }

        if (newAbsolute < 0 || newAbsolute > length_)
            return {-1}; // 越界

        std::streampos newSrcPos = start_ + newAbsolute;
        if (!src_.seekg(newSrcPos))
            return {-1};

        absolutePos_ = newAbsolute;
        setg(nullptr, nullptr, nullptr);
        return {newAbsolute};
    }

    auto seekpos(pos_type pos, std::ios_base::openmode which = std::ios_base::in) -> pos_type override
    {
        return seekoff(pos, std::ios_base::beg, which);
    }

  private:
    std::istream &src_;
    std::streampos start_;
    std::streamsize length_;
    std::streampos absolutePos_;
    std::streampos savedPos_;
    std::array<char, 1> ourBuf_;
};

class OMZipSubStream : public std::istream
{
  public:
    OMZipSubStream(std::istream &src, std::streampos start, std::streamsize length)
        : std::istream(&buf_), buf_(src, start, length)
    {
    }

  private:
    OMZipSubStreamBuf buf_;
};
} // namespace openminecraft::specs::zip

#endif
