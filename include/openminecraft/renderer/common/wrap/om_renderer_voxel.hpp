#ifndef OM_RENDEER_VOXEL_HPP
#define OM_RENDEER_VOXEL_HPP

#include "openminecraft/renderer/common/om_renderer_buffer.hpp"
#include "openminecraft/renderer/common/om_renderer_pipeline.hpp"
#include "openminecraft/renderer/common/om_renderer_task.hpp"
#include "openminecraft/renderer/om_renderer_layer.hpp"
#include <cstdint>
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
class OMVoxelManager
{
  public:
    OMVoxelManager(OMRenderer *renderer, OMRendererRenderTarget *);
    ~OMVoxelManager();

    auto submit(OMRendererTask *) -> OMRendererTask *;

    OMRendererPipeline *pipeline;
    OMRendererBuffer *voxels;
};
} // namespace openminecraft::renderer::common::wrap

#endif
