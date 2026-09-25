#ifndef OM_BLOCK_HPP
#define OM_BLOCK_HPP

#include "openminecraft-shell/data/block/om_blockstate.hpp"
#include "openminecraft-shell/data/block/om_blockstate_registry.hpp"
#include "openminecraft-shell/data/om_identifier.hpp"
#include "openminecraft/renderer/common/wrap/om_renderer_voxel.hpp"
#include <initializer_list>
#include <memory>
#include <unordered_map>
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
using openminecraft::renderer::common::wrap::OMVoxelFacing;
namespace openminecraftshell::data::block
{
class OMBlock : public std::enable_shared_from_this<OMBlock>
{
  public:
    OMBlock() = default;
    ~OMBlock() = default;
    auto isSoild(bool v) -> std::shared_ptr<OMBlock>
    {
        soild = v;
        return shared_from_this();
    }
    auto isFluid(bool v) -> std::shared_ptr<OMBlock>
    {
        fluid = v;
        return shared_from_this();
    }
    auto isTranslucent(bool v) -> std::shared_ptr<OMBlock>
    {
        translucent = v;
        return shared_from_this();
    }

    virtual auto skipsRendering(const OMBlockStateCombined &state, const OMBlockStateCombined &other,
                                OMVoxelFacing direction) -> bool
    {
        return false;
    }

    auto prop(std::string n, std::initializer_list<std::string> values) -> std::shared_ptr<OMBlock>
    {
        properties[n] = values;
        return shared_from_this();
    }
    auto propFacing() -> std::shared_ptr<OMBlock>
    {
        return prop("facing", {"east", "west", "north", "south"});
    }
    auto propInWall() -> std::shared_ptr<OMBlock>
    {
        return prop("in_wall", {"true", "false"});
    }
    auto propOpen() -> std::shared_ptr<OMBlock>
    {
        return prop("open", {"true", "false"});
    }
    auto propPowered() -> std::shared_ptr<OMBlock>
    {
        return prop("powered", {"true", "false"});
    }
    auto propHalf() -> std::shared_ptr<OMBlock>
    {
        return prop("half", {"upper", "lower"});
    }
    auto propWaterlogged() -> std::shared_ptr<OMBlock>
    {
        return prop("water_logged", {"true", "false"});
    }
    auto propHinge() -> std::shared_ptr<OMBlock>
    {
        return prop("hinge", {"left", "right"});
    }
    auto propFace() -> std::shared_ptr<OMBlock>
    {
        return prop("face", {"ceiling", "floor", "wall"});
    }
    auto propRotation() -> std::shared_ptr<OMBlock>
    {
        return prop("rotation", {"0", "1", "2", "3", "4", "5", "6", "7", "8", "9", "10", "11", "12", "13", "14", "15"});
    }
    auto propSnowy() -> std::shared_ptr<OMBlock>
    {
        return prop("snowy", {"true", "false"});
    }

    bool soild = true;
    bool translucent = false;
    bool fluid = false;
    std::unordered_map<std::string, std::vector<std::string>> properties;

    auto operator=(const OMBlock &other) -> OMBlock & = default;
};

class OMTransparentBlock : public OMBlock
{
  public:
    OMTransparentBlock() = default;
    ~OMTransparentBlock() = default;

    auto skipsRendering(const OMBlockStateCombined &state, const OMBlockStateCombined &other, OMVoxelFacing direction)
        -> bool override
    {
        if (state.block == other.block)
        {
            return true;
        }
        return false;
    }
};
} // namespace openminecraftshell::data::block

#endif