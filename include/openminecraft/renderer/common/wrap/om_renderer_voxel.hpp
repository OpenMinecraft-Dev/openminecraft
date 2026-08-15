#ifndef OM_RENDEER_VOXEL_HPP
#define OM_RENDEER_VOXEL_HPP

#include "glm/glm.hpp"
#include "openminecraft/log/om_log_common.hpp"
#include "openminecraft/renderer/common/basics/om_camera.hpp"
#include "openminecraft/renderer/common/om_renderer_buffer.hpp"
#include "openminecraft/renderer/common/om_renderer_pipeline.hpp"
#include "openminecraft/renderer/common/om_renderer_task.hpp"
#include "openminecraft/renderer/om_renderer_layer.hpp"
#include "openminecraft/world/om_world_chunk.hpp"
#include <cstdint>
#include <vector>
namespace openminecraft::renderer::common::wrap
{
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
class OMVoxelManager
{
  public:
    OMVoxelManager(OMRenderer *renderer, OMRendererRenderTarget *);
    ~OMVoxelManager();

    auto compile(world::OMChunk<16> &, std::function<bool(glm::ivec3, int64_t, int64_t, int64_t)>, int chunkid = 0)
        -> std::vector<int>;

    auto submit(OMRendererTask *) -> OMRendererTask *;
    auto update(basics::OMCamera &camera) -> void;

    OMRendererPipeline *pipeline;
    OMRendererBuffer *voxels;
    OMRendererBuffer *chunkoffs;
    int c;

  private:
    log::OMLogger logger;
};
} // namespace openminecraft::renderer::common::wrap

#endif
