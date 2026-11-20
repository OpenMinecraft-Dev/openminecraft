#ifndef OM_BLOCK_REGISTRY_HPP
#define OM_BLOCK_REGISTRY_HPP

#include "openminecraft-shell/data/block/om_block.hpp"
#include "openminecraft-shell/data/om_identifier.hpp"
#include "openminecraft/world/om_world_registry.hpp"
#include <memory>

using namespace openminecraft::world;

namespace openminecraft::shell::data::block
{
extern OMWorldRegistry<OMIdentifier, std::shared_ptr<OMBlock>> &blockRegistry;
void registerBlocks();
} // namespace openminecraft::shell::data::block

#endif