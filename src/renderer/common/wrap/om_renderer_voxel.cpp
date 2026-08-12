#include "openminecraft/renderer/common/wrap/om_renderer_voxel.hpp"
#include "openminecraft/renderer/common/basics/om_vertex_format.hpp"
#include "openminecraft/renderer/common/om_renderer_buffer.hpp"
#include "openminecraft/renderer/common/om_renderer_rendertarget.hpp"
#include "openminecraft/renderer/common/om_renderer_task.hpp"
#include "openminecraft/renderer/om_renderer_layer.hpp"
#include <bitset>
#include <cstdint>
#include <iostream>
#include <vector>

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
auto OMVoxelManager::compile(std::vector<OMVoxel> &lb) -> std::vector<int>
{
    std::vector<int> d;
    std::bitset<16 * 16 * 16> exists = {false};

    for (auto &v : lb)
    {
        exists.set(v.y * 256 + v.x * 16 + v.z);
    }

    auto exist = [&](int x, int y, int z) -> bool {
        if (x < 0 || y < 0 || z < 0 || x > 15 || y > 15 || z > 15)
        {
            return false;
        }

        return exists.test(y * 256 + x * 16 + z);
    };

    for (auto &v : lb)
    {
        if (!exist(v.x, v.y - 1, v.z))
        {
            d.emplace_back(packVoxelPos(v.x, v.y, v.z, 15, 0, OMVoxelFacing::NegY, 0, 0, 0, 0));
            d.emplace_back(packVoxelMetadata(v.texId, 0));
        }
        if (!exist(v.x, v.y + 1, v.z))
        {
            d.emplace_back(packVoxelPos(v.x, v.y, v.z, 15, 0, OMVoxelFacing::PosY, 0, 0, 0, 0));
            d.emplace_back(packVoxelMetadata(v.texId, 0));
        }
        if (!exist(v.x - 1, v.y, v.z))
        {
            d.emplace_back(packVoxelPos(v.x, v.y, v.z, 15, 0, OMVoxelFacing::NegX, 0, 0, 0, 0));
            d.emplace_back(packVoxelMetadata(v.texId, 0));
        }
        if (!exist(v.x + 1, v.y, v.z))
        {
            d.emplace_back(packVoxelPos(v.x, v.y, v.z, 15, 0, OMVoxelFacing::PosX, 0, 0, 0, 0));
            d.emplace_back(packVoxelMetadata(v.texId, 0));
        }
        if (!exist(v.x, v.y, v.z - 1))
        {
            d.emplace_back(packVoxelPos(v.x, v.y, v.z, 15, 0, OMVoxelFacing::NegZ, 0, 0, 0, 0));
            d.emplace_back(packVoxelMetadata(v.texId, 0));
        }
        if (!exist(v.x, v.y, v.z + 1))
        {
            d.emplace_back(packVoxelPos(v.x, v.y, v.z, 15, 0, OMVoxelFacing::PosZ, 0, 0, 0, 0));
            d.emplace_back(packVoxelMetadata(v.texId, 0));
        }
    }

    return d;
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

    std::vector<OMVoxel> datar = {{0, 0, 0, 0}, {1, 0, 1, 0}, {2, 0, 0, 1}, {3, 1, 0, 0}};
    auto data = compile(datar);

    voxels = renderer->allocateBuffer(InstanceData, data.size() * sizeof(uint32_t));
    voxels->updateData(data.data());
    c = data.size();
}
OMVoxelManager::~OMVoxelManager()
{
    delete voxels;
    delete pipeline;
}

auto OMVoxelManager::submit(OMRendererTask *task) -> OMRendererTask *
{
    return task->pipeline(pipeline)->vertexBuffer({voxels})->drawInstanceN(6, c / 2);
}
} // namespace openminecraft::renderer::common::wrap
