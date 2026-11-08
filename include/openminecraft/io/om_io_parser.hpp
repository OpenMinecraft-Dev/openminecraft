#ifndef OM_IO_PARSER_HPP
#define OM_IO_PARSER_HPP

#include <iostream>
#include <memory>
#include <utility>

namespace openminecraft::io
{
class OMParser
{
  public:
    // INFO: only shared_ptr is accepted - the parser must keep the stream alive, otherwise the shared_ptr parameter
    // is destroyed when construction ends and later check()/reads touch a freed stream (SIGSEGV or garbage).
    explicit OMParser(std::shared_ptr<std::istream> stream);
    ~OMParser();
    auto check() -> bool;

    auto stream() const -> std::istream &
    {
        return *sourceOwner;
    }

  protected:
    std::istream *source;
    std::shared_ptr<std::istream> sourceOwner;
};
} // namespace openminecraft::io

#endif
