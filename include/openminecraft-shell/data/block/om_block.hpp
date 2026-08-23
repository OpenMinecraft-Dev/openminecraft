#ifndef OM_BLOCK_HPP
#define OM_BLOCK_HPP

#include "openminecraft-shell/data/block/om_blockstate.hpp"
#include "openminecraft-shell/data/om_identifier.hpp"
#include <initializer_list>
#include <set>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>
namespace openminecraftshell::data::block
{
struct OMBlockModelIdentifier
{
    OMIdentifier location;
    int xrot = 0, yrot = 0, zrot = 0;
    bool uvlock = false;

    OMBlockModelIdentifier() = default;

    auto operator==(const OMBlockModelIdentifier ident) const -> bool
    {
        return location.namesp == ident.location.namesp && location.path == ident.location.path && xrot == ident.xrot &&
               yrot == ident.yrot && zrot == ident.zrot && uvlock == ident.uvlock;
    }
};
} // namespace openminecraftshell::data::block

namespace std
{
template <> struct hash<openminecraftshell::data::block::OMBlockModelIdentifier>
{
    auto operator()(const openminecraftshell::data::block::OMBlockModelIdentifier &id) const noexcept -> std::size_t
    {
        std::size_t seed = 0;
        seed ^=
            std::hash<openminecraftshell::data::OMIdentifier>{}(id.location) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
        seed ^= std::hash<int>{}(id.xrot) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
        seed ^= std::hash<int>{}(id.yrot) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
        seed ^= std::hash<int>{}(id.zrot) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
        seed ^= std::hash<bool>{}(id.uvlock) + 0x9e3779b9 + (seed << 6) + (seed >> 2);

        return seed;
    }
};
} // namespace std
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
    std::unordered_map<OMBlockModelIdentifier, int> requiredModels;
};
} // namespace openminecraftshell::data::block

#endif