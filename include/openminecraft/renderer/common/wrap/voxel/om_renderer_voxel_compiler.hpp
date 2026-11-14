#ifndef OM_RENDERER_VOXEL_COMPILER_HPP
#define OM_RENDERER_VOXEL_COMPILER_HPP

#include "openminecraft/log/om_log_common.hpp"
#include "openminecraft/renderer/common/wrap/voxel/om_renderer_voxel_objects.hpp"
#include <array>
#include <functional>
#include <memory>
#include <tuple>

namespace openminecraft::renderer::common::wrap::voxel
{
// Compute a 6x6 face visibility matrix from the 16x16x16 opaque occupancy grid (bit63 = known)
auto computeSectionVisibility(const std::array<uint8_t, 4096> &blocking) -> uint64_t;

class OMVoxelCompiler
{
  public:
    OMVoxelCompiler();
    ~OMVoxelCompiler();

    auto queryBlockState(const world::OMChunk<16> &, std::function<uint32_t(glm::ivec3, int64_t, int64_t, int64_t)>,
                         int x, int y, int z) -> uint32_t;
    auto existSoild(const world::OMChunk<16> &, std::function<uint32_t(glm::ivec3, int64_t, int64_t, int64_t)>, int x,
                    int y, int z) -> bool;
    auto existFluidSame(const world::OMChunk<16> &, std::function<uint32_t(glm::ivec3, int64_t, int64_t, int64_t)>,
                        int x, int y, int z, int) -> bool;
    auto computeAO(const world::OMChunk<16> &, std::function<uint32_t(glm::ivec3, int64_t, int64_t, int64_t)>, int x,
                   int y, int z, OMVoxelFacing, int bsid, int pid) -> std::tuple<uint8_t, uint8_t, uint8_t, uint8_t>;
    auto checkExistSoild(const world::OMChunk<16> &, std::function<uint32_t(glm::ivec3, int64_t, int64_t, int64_t)>,
                         glm::ivec3, OMVoxelFacing) -> bool;
    auto checkExistFluidSame(const world::OMChunk<16> &, std::function<uint32_t(glm::ivec3, int64_t, int64_t, int64_t)>,
                             glm::ivec3, OMVoxelFacing, int) -> bool;
    auto checkSkip(const world::OMChunk<16> &, std::function<uint32_t(glm::ivec3, int64_t, int64_t, int64_t)>,
                   glm::ivec3, OMVoxelFacing, uint32_t) -> bool;
    auto checkAvgFluid(const world::OMChunk<16> &, std::function<uint32_t(glm::ivec3, int64_t, int64_t, int64_t)>,
                       glm::ivec3, int, int) -> float;
    auto compile(const world::OMChunk<16> &, std::function<uint32_t(glm::ivec3, int64_t, int64_t, int64_t)>,
                 int chunkid, std::function<void(OMVoxel)>, std::function<void(OMVoxelComplex)>,
                 std::function<void(OMVoxelFluid)>, std::function<void(OMVoxel)>, std::function<void(OMVoxelComplex)>,
                 std::function<void(OMVoxelFluid)>) -> void;

    // Visibility-only scan (no mesh generation): fill visibility data for all chunks used by occlusion-culling BFS
    auto computeVisibility(const world::OMChunk<16> &, std::function<uint32_t(glm::ivec3, int64_t, int64_t, int64_t)>)
        -> uint64_t;

    OMVoxelHandler *handler = new OMVoxelHandlerDummy();
    std::function<uint32_t(uint32_t, uint64_t, uint64_t, uint64_t, int, int, int)> modelFetcher;

    std::shared_ptr<OMChunkPaletteProvider> paletteProvider = nullptr;

  private:
    log::OMLogger logger;
};

} // namespace openminecraft::renderer::common::wrap::voxel

#endif
