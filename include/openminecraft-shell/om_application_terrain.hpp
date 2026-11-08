#ifndef OM_APPLICATION_TERRAIN_HPP
#define OM_APPLICATION_TERRAIN_HPP

#include "openminecraft/world/om_world_chunkmanager.hpp"
#include <atomic>
#include <cstdint>

namespace openminecraft::shell::application
{
// Initial camera position: terrain offset in normal mode; above the chunk bounding box in preload mode
struct OMStartupCamera
{
    int64_t chunkX = 0, chunkY = 0, chunkZ = 0;
    float localX = 0.0f, localY = 0.0f, localZ = 0.0f;
    float yaw = 45.0f, pitch = -35.0f;
};

auto startupCamera() -> OMStartupCamera;

// Terrain generation thread body: read from chunks_mapped with OM_CHUNK_PRELOAD, otherwise generate with Perlin
void runTerrainGeneration(world::OMChunkManager<16> &manager, std::atomic<bool> &stopProducing);
} // namespace openminecraft::shell::application

#endif
