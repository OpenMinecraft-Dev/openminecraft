#ifndef OM_IO_TOKENITER_HPP
#define OM_IO_TOKENITER_HPP

#include "openminecraft/io/om_io_parser.hpp"
#include "openminecraft/io/om_io_token.hpp"
#include <memory>

namespace openminecraft::io
{
template <typename T> class OMTokenIter : public OMParser
{
  public:
    OMTokenIter(std::shared_ptr<std::istream> istr) : OMParser(istr.get())
    {
    }
    ~OMTokenIter() = default;

    virtual std::shared_ptr<T> next() = 0;
    virtual bool end() = 0;
};
} // namespace openminecraft::io

#endif
