#ifndef OM_BLOCK_HPP
#define OM_BLOCK_HPP

#include "openminecraft-shell/data/block/om_blockstate.hpp"
#include "openminecraft-shell/data/om_identifier.hpp"
#include <initializer_list>
#include <utility>
#include <vector>
namespace openminecraftshell::data::block
{
class OMBlock
{
  public:
    OMBlock(OMIdentifier name, std::initializer_list<std::pair<OMBlockState, int>> bs)
        : name(std::move(name)), states(bs)
    {
    }

    OMBlock(OMIdentifier name, std::vector<std::pair<OMBlockState, int>> bs)
        : name(std::move(name)), states(std::move(bs))
    {
    }
    OMBlock(OMIdentifier name) : name(std::move(name))
    {
    }
    ~OMBlock() = default;

    const OMIdentifier name;
    std::vector<std::pair<OMBlockState, int>> states;
};
} // namespace openminecraftshell::data::block

#endif