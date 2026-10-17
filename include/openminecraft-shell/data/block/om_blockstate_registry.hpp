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
}; // namespace openminecraft::shell::data::block

namespace openminecraft::shell::data::block
{
extern openminecraft::world::OMWorldRegistry<OMIdentifier, OMBlockStateCombined> blockstateRegistry;

void registerBlockstates();
} // namespace openminecraft::shell::data::block

#endif