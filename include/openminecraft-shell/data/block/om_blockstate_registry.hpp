#ifndef OM_BLOCKSTATE_REGISTRY_HPP
#define OM_BLOCKSTATE_REGISTRY_HPP

#include "openminecraft-shell/data/block/om_blockstate.hpp"
#include "openminecraft-shell/data/om_identifier.hpp"
#include "openminecraft/world/om_world_registry.hpp"
namespace openminecraft::shell::data::block
{
struct OMBlockStateCombined
{
    OMIdentifier block;
    OMBlockState state;
};

inline auto operator==(const OMBlockStateCombined &lhs, const OMBlockStateCombined &rhs) -> bool
{
    return lhs.block == rhs.block && lhs.state == rhs.state;
}

inline auto makeBlockState(const std::string &identifier) -> OMBlockStateCombined
{
    const auto bracket = identifier.find('[');

    if (bracket == std::string::npos)
    {
        return {OMIdentifier(identifier), OMBlockState()};
    }

    const auto end = identifier.rfind(']');
    const std::string props = (end == std::string::npos || end <= bracket + 1)
                                  ? std::string()
                                  : identifier.substr(bracket + 1, end - bracket - 1);

    return {OMIdentifier(identifier.substr(0, bracket)), OMBlockState(props)};
}

inline auto makeBlockState(const OMIdentifier &block, const OMBlockState &state) -> OMBlockStateCombined
{
    return {block, state};
}
}; // namespace openminecraft::shell::data::block

namespace std
{
template <> struct hash<openminecraft::shell::data::block::OMBlockStateCombined>
{
    auto operator()(const openminecraft::shell::data::block::OMBlockStateCombined &c) const noexcept -> std::size_t
    {
        std::size_t h = std::hash<openminecraft::shell::data::OMIdentifier>{}(c.block);
        h ^= c.state.hash() + 0x9e3779b9 + (h << 6) + (h >> 2);
        return h;
    }
};
} // namespace std

namespace openminecraft::shell::data::block
{
extern openminecraft::world::OMWorldRegistry<OMBlockStateCombined, OMBlockStateCombined> &blockStateRegistry;

void registerBlockStates();
} // namespace openminecraft::shell::data::block

#endif