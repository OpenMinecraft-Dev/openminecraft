#ifndef OM_APPLICATION_TERRAIN_HPP
#define OM_APPLICATION_TERRAIN_HPP

#include "openminecraft/world/om_world_chunkmanager.hpp"
#include <atomic>
#include <cstdint>

namespace openminecraft::shell::application
{
// 启动时的相机位置：普通模式放在地形偏移处，预加载模式放在区块包围盒上方
struct OMStartupCamera
{
    int64_t chunkX = 0, chunkY = 0, chunkZ = 0;
    float localX = 0.0f, localY = 0.0f, localZ = 0.0f;
    float yaw = 45.0f, pitch = -35.0f;
};

auto startupCamera() -> OMStartupCamera;

// 地形生成线程主体：OM_CHUNK_PRELOAD 时从 chunks_mapped 读取，否则用 Perlin 生成
void runTerrainGeneration(world::OMChunkManager<16> &manager, std::atomic<bool> &stopProducing);
} // namespace openminecraft::shell::application

#endif
