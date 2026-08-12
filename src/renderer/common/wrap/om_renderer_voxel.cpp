#include "openminecraft/renderer/common/wrap/om_renderer_voxel.hpp"
#include "openminecraft/renderer/common/basics/om_vertex_format.hpp"
#include "openminecraft/renderer/common/om_renderer_buffer.hpp"
#include "openminecraft/renderer/common/om_renderer_rendertarget.hpp"
#include "openminecraft/renderer/common/om_renderer_task.hpp"
#include "openminecraft/renderer/om_renderer_layer.hpp"
#include <array>
#include <cstdint>

namespace openminecraft::renderer::common::wrap
{
// INFO: packed vertex structure in u32
// xxxx yyyy zzzz ssss bbbb ffff a1*2 a2*2 a3*2 a4*2
static auto packVoxelPos(uint8_t x, uint8_t y, uint8_t z, uint8_t skyLight, uint8_t blockLight, OMVoxelFacing facing,
                         uint8_t ao1, uint8_t ao2, uint8_t ao3, uint8_t ao4) -> int
{
    return x << 28 | y << 24 | z << 20 | skyLight << 16 | blockLight << 12 | facing << 8 | ao1 << 6 | ao2 << 4 |
           ao3 << 2 | ao4;
}
// INFO: packed vertex metadata in u32
// DS*16 CS*16
static auto packVoxelMetadata(uint16_t tex, uint16_t chkid) -> int
{
    return tex << 16 | chkid;
}
OMVoxelManager::OMVoxelManager(OMRenderer *renderer, OMRendererRenderTarget *target)
{
    basics::OMVertexFormat format;
    format.setInstance()
        ->appendPart("voxelPos", basics::Integer)
        ->appendPart("voxelMetadata", basics::Integer)
        ->nextGroup()
        ->decideStruct();
    auto voxelFrg = renderer->shaderManager.preprocess("core/voxel.frag.glsl", Fragment, GLSLSource, format);
    auto voxelVtx = renderer->shaderManager.preprocess("core/voxel.vert.glsl", Vertex, GLSLSource, format);

    pipeline = renderer->createPipeline()
                   ->input(UniformBuffer)
                   ->inputName("ObjectInfo")
                   ->input(UniformBuffer)
                   ->inputName("Camera")
                   ->input(UniformBuffer)
                   ->inputName("Lighting")
                   ->input(ImageSampler)
                   ->inputName("inTexture")
                   ->output(target)
                   ->samples(4)
                   ->setCullMode(renderer::common::Back)
                   ->setFrontClockwise(true)
                   ->shader(voxelFrg)
                   ->shader(voxelVtx)
                   ->format(format)
                   ->blendFunc({Alpha, OneMinusAlpha, Alpha, OneMinusAlpha})
                   ->blend(true)
                   ->depth(true, true)
                   ->depthReverseZ(true)
                   ->buildN();

    voxels = renderer->allocateBuffer(InstanceData, 6 * 2 * sizeof(uint32_t));
    std::array<int, 6 * 2> data = {
        packVoxelPos(5, 0, 0, 15, 0, NegX, 0, 0, 0, 0), packVoxelMetadata(3, 0),
        packVoxelPos(5, 0, 0, 15, 0, PosX, 0, 0, 0, 0), packVoxelMetadata(3, 0),
        packVoxelPos(5, 0, 0, 15, 0, NegY, 0, 0, 0, 0), packVoxelMetadata(3, 0),
        packVoxelPos(5, 0, 0, 15, 0, PosY, 0, 0, 0, 0), packVoxelMetadata(3, 0),
        packVoxelPos(5, 0, 0, 15, 0, NegZ, 0, 0, 0, 0), packVoxelMetadata(3, 0),
        packVoxelPos(5, 0, 0, 15, 0, PosZ, 0, 0, 0, 0), packVoxelMetadata(3, 0),
    };
    voxels->updateData(data.data());
}
OMVoxelManager::~OMVoxelManager()
{
    delete voxels;
    delete pipeline;
}

auto OMVoxelManager::submit(OMRendererTask *task) -> OMRendererTask *
{
    return task->pipeline(pipeline)->vertexBuffer({voxels})->drawInstanceN(6, 6);
}
} // namespace openminecraft::renderer::common::wrap
