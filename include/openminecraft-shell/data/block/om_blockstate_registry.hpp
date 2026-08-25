#ifndef OM_BLOCKSTATE_REGISTRY_HPP
#define OM_BLOCKSTATE_REGISTRY_HPP

#include "openminecraft-shell/data/block/om_block.hpp"
#include "openminecraft-shell/data/block/om_blockstate.hpp"
#include "openminecraft-shell/data/om_identifier.hpp"
#include "openminecraft/world/om_world_registry.hpp"
namespace openminecraftshell::data::block
{
struct OMBlockStateCombined
{
    OMIdentifier block;
    OMBlockState state;
};
}; // namespace openminecraftshell::data::block

namespace openminecraftshell::data::block
{
extern openminecraft::world::OMWorldRegistry<OMIdentifier, OMBlockStateCombined> blockstateRegistry;

void registerBlockstates();
} // namespace openminecraftshell::data::block

#endif