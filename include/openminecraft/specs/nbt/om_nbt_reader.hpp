#ifndef OM_NBT_READER_HPP
#define OM_NBT_READER_HPP

#include "openminecraft/specs/nbt/om_nbt.hpp"
#include <memory>
#include <iostream>
#include <utility>
namespace openminecraft::specs::nbt
{
class OMNbtReader
{
  public:
    OMNbtReader(std::shared_ptr<std::istream> istr) : istr(std::move(istr))
    {
    }

    auto parse(bool haveName = true) -> std::shared_ptr<OMNbtTag>;
    auto readPrefixString() -> std::string;

  private:
    std::shared_ptr<std::istream> istr;
};
} // namespace openminecraft::specs::nbt

#endif