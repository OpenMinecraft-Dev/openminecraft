#ifndef OM_COMPILER_TOKENITER_HPP
#define OM_COMPILER_TOKENITER_HPP

#include "openminecraft/compiler/om_compiler_parser.hpp"
#include <memory>
#include <utility>

namespace openminecraft::compiler
{
constexpr const char allocatorId[] = "compiler_parser";

template <typename T> class OMTokenIter : public OMParser
{
  public:
    OMTokenIter(std::shared_ptr<std::istream> istr) : OMParser(std::move(istr))
    {
    }
    ~OMTokenIter() = default;

    virtual auto next() -> std::shared_ptr<T> = 0;
    virtual auto end() -> bool = 0;
};
} // namespace openminecraft::compiler

#endif
