#ifndef OM_UTIL_MEMSTREAM_HPP
#define OM_UTIL_MEMSTREAM_HPP

#include <iostream>

namespace openminecraft::util
{
class OMMemoryStreamBuf : public std::streambuf
{
  public:
    OMMemoryStreamBuf(const char *data, std::size_t size)
    {
        char *begin = const_cast<char *>(data);
        setg(begin, begin, begin + size);
    }

  protected:
    auto seekoff(off_type off, std::ios_base::seekdir dir, std::ios_base::openmode which = std::ios_base::in)
        -> pos_type override
    {
        if (!(which & std::ios_base::in))
            return {off_type(-1)};

        off_type base;
        if (dir == std::ios_base::beg)
            base = 0;
        else if (dir == std::ios_base::cur)
            base = gptr() - eback();
        else if (dir == std::ios_base::end)
            base = egptr() - eback();
        else
            return {off_type(-1)};

        return seekpos(pos_type(base + off), std::ios_base::in);
    }

    auto seekpos(pos_type pos, std::ios_base::openmode which = std::ios_base::in) -> pos_type override
    {
        if (!(which & std::ios_base::in))
            return {off_type(-1)};

        off_type offset = pos - pos_type(off_type(0));
        off_type size = egptr() - eback();

        if (offset < 0 || offset > size)
            return {off_type(-1)};

        setg(eback(), eback() + offset, egptr());
        return {offset};
    }
};

class OMMemoryStream : public std::istream
{
  public:
    OMMemoryStream(const char *src, std::size_t length) : std::istream(&buf_), buf_(src, length)
    {
    }

  private:
    OMMemoryStreamBuf buf_;
};
} // namespace openminecraft::util

#endif
