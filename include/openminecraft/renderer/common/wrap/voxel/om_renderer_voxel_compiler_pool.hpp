#ifndef OM_RENDERER_VOXEL_COMPILER_POOL_HPP
#define OM_RENDERER_VOXEL_COMPILER_POOL_HPP

#include "openminecraft/renderer/common/wrap/voxel/om_renderer_voxel_compiler.hpp"
#include "openminecraft/renderer/common/wrap/voxel/om_renderer_voxel_layer.hpp"
#include "openminecraft/world/om_world_chunkmanager.hpp"
#include <functional>
#include <mutex>
#include <queue>
#include <thread>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace openminecraft::renderer::common::wrap::voxel
{
class OMVoxelCompilerPool
{
  public:
    OMVoxelCompilerPool(world::OMChunkManager<16> &, OMVoxelCompiler &, int = 4);
    ~OMVoxelCompilerPool();

    void upload(OMVoxelLayer<OMVoxel> *cutout, OMVoxelLayer<OMVoxelComplex> *cutoutComplex,
                OMVoxelLayer<OMVoxelFluid> *fluid, OMVoxelLayer<OMVoxel> *translucent,
                OMVoxelLayer<OMVoxelComplex> *translucentComplex, OMVoxelLayer<OMVoxelFluid> *translucentFluid,
                const std::function<void(int, world::OMChunkIndex, uint64_t)> &visSink);
    void compile(int i, bool useCache = false, float distanceSq = 3.4e38f);
    // Lightweight visibility request (no geometry; only fills data for occlusion culling)
    void requestVisibility(int i);
    void dropCache(int i)
    {
        std::lock_guard g(bufferMutex);
        cached.erase(i);
        cachedOrigin.erase(i);
        cutoutC.erase(i);
        cutoutComplexC.erase(i);
        translucentC.erase(i);
        translucentComplexC.erase(i);
        visC.erase(i);
        visPending.erase(i);
    }

  private:
    std::unordered_map<int, bool> cached;
    // Cache the chunk coordinates: slots are recycled by chunkManager,
    // so the cache is valid only when coordinates match
    std::unordered_map<int, world::OMChunkIndex> cachedOrigin;
    world::OMChunkManager<16> &manager;
    OMVoxelCompiler &compiler;

    std::mutex poolMutex;

    // Compile requests: sorted by squared distance to the camera (nearest first)
    struct OMCompileRequest
    {
        float distanceSq = 0.0f;
        int chunk = -1;
        bool useCache = false;

        bool operator>(const OMCompileRequest &other) const
        {
            return distanceSq > other.distanceSq;
        }
    };

    std::priority_queue<OMCompileRequest, std::vector<OMCompileRequest>, std::greater<>> queuedChunks;
    std::vector<int> visRequests;
    std::unordered_set<int> visQueued;
    // Keep one entry per chunk in the queue (fast rotation repeatedly requests the same chunk)
    std::unordered_set<int> queuedIndex;

    std::mutex bufferMutex;
    std::unordered_map<int, std::vector<OMVoxel>> cutout, translucent;
    std::unordered_map<int, std::vector<OMVoxelComplex>> cutoutComplex, translucentComplex;
    std::unordered_map<int, std::vector<OMVoxelFluid>> fluid, translucentFluid;
    // Per-section 6x6 face visibility matrix: bit(f*6 + t) means you can see in through face f and exit through face t
    std::unordered_map<int, uint64_t> visC;
    std::unordered_map<int, std::pair<world::OMChunkIndex, uint64_t>> visPending;

    std::unordered_map<int, std::vector<OMVoxel>> cutoutC, translucentC;
    std::unordered_map<int, std::vector<OMVoxelComplex>> cutoutComplexC, translucentComplexC;
    std::unordered_map<int, std::vector<OMVoxelFluid>> fluidC, translucentFluidC;

    std::vector<std::thread *> thrs;
    bool active = true;
};

} // namespace openminecraft::renderer::common::wrap::voxel

#endif
