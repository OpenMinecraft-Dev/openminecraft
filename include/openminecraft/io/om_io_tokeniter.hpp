#ifndef OM_IO_TOKENITER_HPP
#define OM_IO_TOKENITER_HPP

#include "openminecraft/io/om_io_parser.hpp"
#include <memory>

namespace openminecraft::io
{
constexpr const char allocatorId[] = "io_parser";

template <typename T> class OMTokenIter : public OMParser
{
  public:
    OMTokenIter(std::shared_ptr<std::istream> istr) : OMParser(istr.get())
    {
    }
    ~OMTokenIter() = default;

    virtual auto next() -> std::shared_ptr<T> = 0;
    virtual auto end() -> bool = 0;
};
} // namespace openminecraft::io

#endif
