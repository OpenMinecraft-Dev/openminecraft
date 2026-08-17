#ifndef OM_RENDEER_VOXEL_HPP
#define OM_RENDEER_VOXEL_HPP

#include "glm/fwd.hpp"
#include "glm/glm.hpp"
#include "openminecraft/log/om_log_common.hpp"
#include "openminecraft/renderer/common/basics/om_camera.hpp"
#include "openminecraft/renderer/common/om_renderer_buffer.hpp"
#include "openminecraft/renderer/common/om_renderer_pipeline.hpp"
#include "openminecraft/renderer/common/om_renderer_task.hpp"
#include "openminecraft/renderer/common/om_renderer_texture.hpp"
#include "openminecraft/renderer/common/wrap/om_renderer_segbuf.hpp"
#include "openminecraft/renderer/om_renderer_layer.hpp"
#include "openminecraft/world/om_world_chunk.hpp"
#include <cstdint>
#include <list>
#include <unordered_map>
#include <vector>
namespace openminecraft::renderer::common::wrap
{
struct OMChunkIndex
{
    int64_t x, y, z;

    auto operator==(const OMChunkIndex &c) const -> bool
    {
        return x == c.x && y == c.y && z == c.z;
    }
};
}; // namespace openminecraft::renderer::common::wrap

namespace std
{
template <> struct hash<openminecraft::renderer::common::wrap::OMChunkIndex>
{
    size_t operator()(const openminecraft::renderer::common::wrap::OMChunkIndex &c) const noexcept
    {
        uint64_t hash = std::hash<int64_t>{}(c.x);
        hash ^= std::hash<int64_t>{}(c.y) + 0x9e3779b9 + (hash << 6) + (hash >> 2);
        hash ^= std::hash<int64_t>{}(c.z) + 0x9e3779b9 + (hash << 6) + (hash >> 2);
        return hash;
    }
};
} // namespace std

namespace openminecraft::renderer::common::wrap
{
struct OMVoxel
{
    int32_t voxelBasic;
    int32_t voxelMeta;
    int32_t voxelExtra;
};
enum OMVoxelFacing : uint8_t
{
    NegX = 0b000,
    PosX = 0b100,
    NegZ = 0b001,
    PosZ = 0b101,
    NegY = 0b010,
    PosY = 0b110
};
// INFO: note for the ambient occulusion
// Type -> (AO1, AO2, AO3, AO4)
// NegX => ((0, 1, 0), (0, 0, 0), (0, 1, 1), (0, 0, 1))
// PosX => ((1, 1, 1), (1, 0, 1), (1, 1, 0), (1, 0, 0))
// NegZ => ((1, 1, 0), (1, 0, 0), (0, 1, 0), (0, 0, 0))
// PosZ => ((0, 1, 1), (0, 0, 1), (1, 1, 1), (1, 0, 1))
// NegY => ((0, 0, 0), (0, 0, 1), (1, 0, 0), (1, 0, 1))
// PosY => ((0, 1, 0), (0, 1, 1), (1, 1, 0), (1, 1, 1))
class OMVoxelCompiler
{
  public:
    OMVoxelCompiler();
    ~OMVoxelCompiler();

    auto compile(world::OMChunk<16> &, std::function<bool(glm::ivec3, int64_t, int64_t, int64_t)>, int chunkid,
                 std::function<void(OMVoxel)>) -> void;

  private:
    log::OMLogger logger;
};

class OMVoxelManager
{
  public:
    OMVoxelManager(OMRenderer *renderer, OMRendererRenderTarget *, OMRendererTexture *);
    ~OMVoxelManager();

    auto submit(OMRendererTask *) -> OMRendererTask *;
    auto update(basics::OMCamera &camera) -> void;

    OMRendererPipeline *pipeline;
    OMRendererSegBuf *voxels;
    OMRendererBuffer *chunkoffs;
    OMRendererTexture *textureAtlas;
    int faceCount;

  private:
    log::OMLogger logger;
    OMVoxelCompiler compiler;

    std::vector<world::OMChunk<16>> chunks;
    std::vector<std::list<OMRendererSegBufBlock>> chunkBlocks = {};
    std::unordered_map<OMChunkIndex, int> chunkMap;

    void compile(int i);
};
} // namespace openminecraft::renderer::common::wrap

#endif
