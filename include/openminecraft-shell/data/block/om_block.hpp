#ifndef OM_BLOCK_HPP
#define OM_BLOCK_HPP

#include "openminecraft-shell/data/block/om_blockstate.hpp"
#include "openminecraft-shell/data/om_identifier.hpp"
#include "openminecraft/io/json/om_io_ast_json.hpp"
#include <memory>
#include <unordered_map>
#include <utility>
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
    OMBlock(std::unordered_map<OMBlockState, int> bs) : states(std::move(bs))
    {
    }

    OMBlock() = default;
    ~OMBlock() = default;
    auto isSoild(bool v) -> OMBlock &
    {
        soild = v;
        return *this;
    }

    std::unordered_map<OMBlockState, int> states;
    std::unordered_map<OMBlockModelIdentifier, int> requiredModels;
    std::shared_ptr<openminecraft::io::json::OMJsonNode> resolverCache = nullptr;
    bool soild = true;

    auto operator=(const OMBlock &other) -> OMBlock & = default;
};
} // namespace openminecraftshell::data::block

#endif