#ifndef OM_BIOME_REGISTRY_HPP
#define OM_BIOME_REGISTRY_HPP

#include "openminecraft-shell/data/biome/om_biome.hpp"
#include "openminecraft-shell/data/om_identifier.hpp"
#include "openminecraft/world/om_world_registry.hpp"
namespace openminecraft::shell::data::biome
{
extern world::OMWorldRegistry<OMIdentifier, std::shared_ptr<OMBiome>> &biomeRegistry;
void registerBiomes();
} // namespace openminecraft::shell::data::biome

#endif